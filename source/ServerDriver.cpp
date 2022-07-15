#include "openvr_driver.h"

#include "ServerDriver.h"

#include <windows.h>
#include <shellapi.h>
#include <tchar.h>

#pragma comment(lib,"shell32.lib")

#pragma comment(lib, "User32.lib")


std::thread ErrorAlarmThreadWorker;
std::thread BoardcastThreadWorker;
void ErrorAlarm(HyResult result);
void Boardcast();

vr::EVRInitError ServerDriver::Init(vr::IVRDriverContext* DriverContext) {

	vr::EVRInitError eError = vr::InitServerDriverContext(DriverContext);
		if (eError != vr::VRInitError_None) {
			return eError;
	}
	HyStartup();
	HyResult ifCreate = HyCreateInterface(HyDevice_InterfaceName, 0, &HyTrackingDevice);
	
	//BoardcastThreadWorker = std::thread::thread(&Boardcast);
	if (ifCreate >= 100) {//we got an error..
		ErrorAlarmThreadWorker = std::thread::thread(&ErrorAlarm, ifCreate);
		return vr::VRInitError_Driver_Failed;
	}
	
	this->HyLeftController = new HyController("LctrTEST@LXWX", TrackedControllerRole_LeftHand,HyTrackingDevice);
	this->HyRightController = new HyController("RctrTEST@LXWX", TrackedControllerRole_RightHand,HyTrackingDevice);

	vr::VRServerDriverHost()->TrackedDeviceAdded(HyLeftController->GetSerialNumber().c_str(), vr::TrackedDeviceClass_Controller, this->HyLeftController);
	vr::VRServerDriverHost()->TrackedDeviceAdded(HyRightController->GetSerialNumber().c_str(), vr::TrackedDeviceClass_Controller, this->HyRightController);

	m_bEventThreadRunning = false;
	if (!m_bEventThreadRunning)
	{
		m_bEventThreadRunning = true;
		send_haptic_thread_worker = std::thread::thread(&ServerDriver::Send_haptic_event_thread, this);
		send_haptic_thread_worker.detach();
		updatePoseThreadWorker = std::thread::thread(&ServerDriver::UpdatePoseThread, this);
		updatePoseThreadWorker.detach();
		updateKeyThreadWorker = std::thread::thread(&ServerDriver::UpdateKeyThread, this);
		updateKeyThreadWorker.detach();
		checkBatteryThreadWorker = std::thread::thread(&ServerDriver::UpdateControllerBatteryThread, this);
		checkBatteryThreadWorker.detach();
	}
	return vr::VRInitError_None;
}

void ErrorAlarm(HyResult result) {
	switch (result)
	{
	case hyError_NeedStartup:
		MessageBox(NULL, L"hyError_NeedStartup\nδ��ʼ��", L"����", MB_OK);
		break;
	case hyError_DeviceNotStart:
		MessageBox(NULL, L"hyError_DeviceNotStart\n�豸δ����", L"����", MB_OK);
		break;
	case hyError_InvalidHeadsetOrientation:
		MessageBox(NULL, L"hyError_InvalidHeadsetOrientation\n��Ч��ͷ����ת����", L"����", MB_OK);
		break;
	case hyError_RenderNotCreated:
		MessageBox(NULL, L"hyError_RenderNotCreated\nδ������Ⱦ���", L"����", MB_OK);
		break;
	case hyError_TextureNotCreated:
		MessageBox(NULL, L"hyError_TextureNotCreated\nδ��������", L"����", MB_OK);
		break;
	case hyError_DisplayLost:
		MessageBox(NULL, L"hyError_DisplayLost\n��ʾ�ӿڶ�ʧ", L"����", MB_OK);
		break;
	case hyError_NoHmd:
		MessageBox(NULL, L"hyError_NoHmdFound\nδ����ͷ��", L"����", MB_OK);
		break;
	case hyError_DeviceNotConnected:
		MessageBox(NULL, L"hyError_DeviceNotConnected\n�豸δ����", L"����", MB_OK);
		break;
	case hyError_ServiceConnection:
		MessageBox(NULL, L"hyError_ServiceConnection\n�������Ӵ���", L"����", MB_OK);
		break;
	case hyError_ServiceError:
		MessageBox(NULL, L"hyError_ServiceError\n�������", L"����", MB_OK);
		break;
	case hyError_InvalidParameter:
		MessageBox(NULL, L"hyError_InvalidParameter\n��Ч����", L"����", MB_OK);
		break;
	case hyError_NoCalibration:
		MessageBox(NULL, L"hyErrrr_NoCalibration\n��Ҫ��HY�ͻ��˽���У׼", L"����", MB_OK);
		break;
	case hyError_NotImplemented:
		MessageBox(NULL, L"hyError_NotImplemented\nδʵ����", L"����", MB_OK);
		break;
	case hyError_InvalidClientType:
		MessageBox(NULL, L"hyError_InvalidClientType\n��Ч�Ŀͻ�������", L"����", MB_OK);
		break;
	case hyError_BufferTooSmall:
		MessageBox(NULL, L"hyError_BufferTooSmall\n��������С", L"����", MB_OK);
		break;
	case hyError_InvalidState:
		MessageBox(NULL, L"hyError_InvalidState\n�豸״̬��Ч", L"����", MB_OK);
		break;
	default:
		break;
	}
}

void Boardcast() {
	const TCHAR szOperation[] = _T("open");
	const TCHAR szAddress[] = _T("https://github.com/lixiangwuxian/HyperealDriverTest");
	int result=MessageBox(NULL, L"2022/7/1 beta 0.4����Hypereal��SteamVR��������ʹ�á�\n\
Bug:���𶯡�\n\
Created By lixiangwuxian@github\n\
�����ɴ򿪴�����Githubҳ�������\n\
����Ǽ���ʹ��"\
, L"��ʾ", MB_YESNO);
	if (result == IDNO) {
		ShellExecute(NULL, szOperation, szAddress, NULL, NULL, SW_SHOWNORMAL);
	}
}

void ServerDriver::Cleanup() {
	this->HyTrackingDevice=NULL;
	this->HyLeftController=NULL;
	this->HyRightController=NULL;

	delete this->HyTrackingDevice;
	delete this->HyLeftController;
	delete this->HyRightController;
	VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

const char* const* ServerDriver::GetInterfaceVersions() {
	return vr::k_InterfaceVersions;
}


bool ServerDriver::ShouldBlockStandbyMode() {
	return false;
}



void ServerDriver::UpdateHaptic(VREvent_t& eventHandle)
{
	if (eventHandle.eventType == VREvent_Input_HapticVibration)
	{
		float amplitude=0, duration=0;
		VREvent_HapticVibration_t data = eventHandle.data.hapticVibration;
		duration = fmaxf(10,data.fDurationSeconds*1000);
		amplitude =fmaxf(0.3,data.fAmplitude);
		if (HyLeftController->GetPropertyContainer() == data.containerHandle) {
			HyTrackingDevice->SetControllerVibration(HY_SUBDEV_CONTROLLER_LEFT,duration, amplitude);
		}
		else if (HyRightController->GetPropertyContainer() == data.containerHandle) {
			HyTrackingDevice->SetControllerVibration(HY_SUBDEV_CONTROLLER_RIGHT, duration, amplitude);
		}
	}
}

void ServerDriver::UpdateHyPose(const HyTrackingState& newData,bool leftOrRight) {
	if (leftOrRight){
		HyLeftController->UpdatePose(newData);
	}else{
		HyRightController->UpdatePose(newData);
	}
}

void ServerDriver::UpdateHyKey(HySubDevice device, HyInputState type)
{
	if (device == HY_SUBDEV_CONTROLLER_LEFT) {
		HyLeftController->SendButtonUpdate(type);
	}
	else if(device == HY_SUBDEV_CONTROLLER_RIGHT){
		HyRightController->SendButtonUpdate(type);
	}
}

void ServerDriver::UpdateControllerBatteryThread()
{
	int64_t batteryValue = 3;
	while (true) {
		HyTrackingDevice->GetIntValue(HY_PROPERTY_DEVICE_BATTERY_INT, batteryValue, HY_SUBDEV_CONTROLLER_LEFT);
		HyLeftController->UpdateBattery(batteryValue);
		HyTrackingDevice->GetIntValue(HY_PROPERTY_DEVICE_BATTERY_INT, batteryValue, HY_SUBDEV_CONTROLLER_RIGHT);
		HyRightController->UpdateBattery(batteryValue);
		Sleep(1000);
	}
}


void ServerDriver::Send_haptic_event_thread()
{
	VREvent_t pEventHandle;
	bool bHasEvent = false;
	while (m_bEventThreadRunning)
	{
		bHasEvent = vr::VRServerDriverHost()->PollNextEvent(&pEventHandle, sizeof(VREvent_t));
		if (bHasEvent)
		{
			UpdateHaptic(pEventHandle);
		}
		else
		{
			Sleep(1);
		}
		memset(&pEventHandle, 0, sizeof(VREvent_t));
	}
}


void ServerDriver::UpdatePoseThread() {
	while (m_bEventThreadRunning) {
		HyTrackingDevice->GetTrackingState(HY_SUBDEV_CONTROLLER_LEFT, 0, trackInform,0);
		UpdateHyPose(trackInform, true);
		HyTrackingDevice->GetTrackingState(HY_SUBDEV_CONTROLLER_RIGHT, 0, trackInform,0);
		UpdateHyPose(trackInform, false);
		Sleep(8);
	}
}

void ServerDriver::UpdateKeyThread() {
	HyInputState keyInput;
	while(m_bEventThreadRunning) {
		HyTrackingDevice->GetControllerInputState(HY_SUBDEV_CONTROLLER_LEFT, keyInput);
		UpdateHyKey(HY_SUBDEV_CONTROLLER_LEFT, keyInput);
		HyTrackingDevice->GetControllerInputState(HY_SUBDEV_CONTROLLER_RIGHT, keyInput);
		UpdateHyKey(HY_SUBDEV_CONTROLLER_RIGHT, keyInput);
		Sleep(1);
	}
}

void ServerDriver::RunFrame() {}
void ServerDriver::EnterStandby() {}
void ServerDriver::LeaveStandby() {}