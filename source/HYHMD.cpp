#include "HYHMD.h"
#include <thread>
#pragma comment(lib,"d3d11.lib")
using namespace vr;

HyHMD::HyHMD(std::string id, HyDevice* Device) {
	m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;
	m_sSerialNumber = id;
	m_sModelNumber = "";
	HMDDevice = Device;
	initDisplayConfig();
	D3D_FEATURE_LEVEL eFeatureLevel;
	D3D_FEATURE_LEVEL pFeatureLevels[2]{};
	pFeatureLevels[0] = D3D_FEATURE_LEVEL_11_1;
	pFeatureLevels[1] = D3D_FEATURE_LEVEL_11_0;
	D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, pFeatureLevels, 2, D3D11_SDK_VERSION, &pD3D11Device, &eFeatureLevel, &pD3D11DeviceContext);
	memset(&m_DispDesc, 0, sizeof(HyGraphicsContextDesc));
	m_DispDesc.m_graphicsDevice = pD3D11Device;
	m_DispDesc.m_graphicsAPI = HY_GRAPHICS_D3D11;
	m_DispDesc.m_pixelFormat = HY_TEXTURE_R8G8B8A8_UNORM_SRGB;
	m_DispDesc.m_pixelDensity = 1.0f;
	m_DispDesc.m_mirrorWidth = 2160;
	m_DispDesc.m_mirrorHeight = 1200;
	m_DispDesc.m_flags = 0;
	HyResult hr=Device->CreateGraphicsContext(m_DispDesc, &m_DispHandle);
	m_DispTexDesc.m_uvOffset = HyVec2{ 0.0f, 0.0f };
	m_DispTexDesc.m_uvSize = HyVec2{ 1.0f, 1.0f };
}


HyHMD::~HyHMD(){
	m_DispHandle->Release();
}


void HyHMD::initDisplayConfig() {
	m_nWindowX = 0;
	m_nWindowY = 0;
	m_nWindowWidth = 2160;
	m_nWindowHeight = 1200;
#ifdef DISPLAY_DEBUG
	m_nWindowWidth /= 10;
	m_nWindowHeight /= 10;//smaller window
#endif // DISPLAY_DEBUG
}

EVRInitError HyHMD::Activate(uint32_t unObjectId)
{
	m_unObjectId = unObjectId;
	initPos();
	m_ulPropertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);
	vr::VRProperties()->SetUint64Property(m_ulPropertyContainer, Prop_CurrentUniverseId_Uint64, 2);
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_SerialNumber_String, m_sSerialNumber.c_str());
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ModelNumber_String, "HYHMD");
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ManufacturerName_String, "HYPEREAL");
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_UserIpdMeters_Float, 0.068);//soft ipd
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_UserHeadToEyeDepthMeters_Float, 0.16f);
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_DisplayFrequency_Float, 90);
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_SecondsFromVsyncToPhotons_Float, 0.0);
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_NamedIconPathDeviceOff_String, "{revive_hypereal}/icons/hypereal_headset_off.png");
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_NamedIconPathDeviceSearching_String, "{revive_hypereal}/icons/hypereal_headset_searching.gif");
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_NamedIconPathDeviceSearchingAlert_String, "{revive_hypereal}/icons/hypereal_headset_searching_alert.gif");
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_NamedIconPathDeviceReady_String, "{revive_hypereal}/icons/hypereal_headset_ready.png");
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_NamedIconPathDeviceReadyAlert_String, "{revive_hypereal}/icons/hypereal_headset_ready_alert.png");
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_NamedIconPathDeviceNotReady_String, "{revive_hypereal}/icons/hypereal_headset_not_ready.png");
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_NamedIconPathDeviceStandby_String, "{revive_hypereal}/icons/hypereal_headset_standby.png");
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_NamedIconPathDeviceStandbyAlert_String, "{revive_hypereal}/icons/hypereal_headset_ready_alert.png");
	return VRInitError_None;
}

void HyHMD::Deactivate()
{
	m_unObjectId = k_unTrackedDeviceIndexInvalid;
}

void HyHMD::EnterStandby()
{
}

void* HyHMD::GetComponent(const char* pchComponentNameAndVersion) {
	if (_stricmp(pchComponentNameAndVersion, ITrackedDeviceServerDriver_Version) == 0) {
		return static_cast<ITrackedDeviceServerDriver*>(this);
	}
	if (_stricmp(pchComponentNameAndVersion, IVRDisplayComponent_Version) == 0) {
		return static_cast<IVRDisplayComponent*>(this);
	}
	if (_stricmp(pchComponentNameAndVersion, IVRVirtualDisplay_Version) == 0) {
#ifndef DISPLAY_DEBUG
		return static_cast<IVRVirtualDisplay*>(this);//commit this to get display on screen
#endif
	}
	return nullptr;
}

void HyHMD::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize)
{
	if (unResponseBufferSize >= 1)
		pchResponseBuffer[0] = 0;
}

DriverPose_t HyHMD::GetPose()
{
	return m_Pose;
}

PropertyContainerHandle_t HyHMD::GetPropertyContainer()
{
	return PropertyContainerHandle_t();
}

std::string HyHMD::GetSerialNumber()
{
	return m_sSerialNumber;
}

void HyHMD::UpdatePose(HyTrackingState HMDData)
{
	vr::VRServerDriverHost()->TrackedDevicePoseUpdated(m_unObjectId, GetPose(HMDData), sizeof(DriverPose_t));
}

//display component

void HyHMD::GetWindowBounds(int32_t* pnX, int32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight)
{
	*pnX = 0;
	*pnY = 0;
	*pnWidth = m_nWindowWidth;
	*pnHeight = m_nWindowHeight;
}

bool HyHMD::IsDisplayOnDesktop()
{
	return false;
}

bool HyHMD::IsDisplayRealDisplay()
{
	return false;
}

void HyHMD::GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight)
{
	uint32_t leftWidth = 0, leftHeight = 0;
	uint32_t rightWidth = 0, rightHeight = 0;
	m_DispHandle->GetRenderTargetSize(HY_EYE_LEFT, leftWidth, leftHeight);
	m_DispHandle->GetRenderTargetSize(HY_EYE_RIGHT, rightWidth, rightHeight);
	uint32_t finalWidth = leftWidth + rightWidth;
	uint32_t finalHeight = leftHeight > rightHeight ? leftHeight : rightHeight;
	*pnWidth = finalWidth/2;//for single eye
	*pnHeight = finalHeight;
}

void HyHMD::GetEyeOutputViewport(EVREye eEye, uint32_t* pnX, uint32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight)
{
	*pnY = 0;
	*pnWidth = m_nWindowWidth / 2;
	*pnHeight = m_nWindowHeight;

	if (eEye == vr::Eye_Left) {
		*pnX = 0;
	}
	else {
		*pnX = m_nWindowWidth / 2;
	}

}

void HyHMD::GetProjectionRaw(EVREye eEye, float* pfLeft, float* pfRight, float* pfTop, float* pfBottom)
{
	//set fov
	HyFov fov;
	if (eEye == Eye_Left) {
		HMDDevice->GetFloatArray(HY_PROPERTY_HMD_LEFT_EYE_FOV_FLOAT4_ARRAY, fov.val, 4);
		*pfLeft = -fov.m_leftTan;
		*pfRight = fov.m_rightTan;
		*pfTop = -fov.m_upTan;
		*pfBottom = fov.m_downTan;
	}
	else if (eEye == Eye_Right) {
		HMDDevice->GetFloatArray(HY_PROPERTY_HMD_RIGHT_EYE_FOV_FLOAT4_ARRAY, fov.val, 4);
		*pfLeft = -fov.m_leftTan;
		*pfRight = fov.m_rightTan;
		*pfTop = -fov.m_upTan;
		*pfBottom = fov.m_downTan;
	}
}

DistortionCoordinates_t HyHMD::ComputeDistortion(EVREye eEye, float fU, float fV)
{
	DistortionCoordinates_t coordinates;
	//from https://github.com/HelenXR/openvr_survivor/blob/master/src/head_mount_display_device.cc
	float hX;
	float hY;
	double rr;
	double r2;
	double theta;
	rr = sqrt((fU - 0.5f) * (fU - 0.5f) + (fV - 0.5f) * (fV - 0.5f));
	r2 = rr * (1 + m_fDistortionK1 * (rr * rr) +
		m_fDistortionK2 * (rr * rr * rr * rr));
	theta = atan2(fU - 0.5f, fV - 0.5f);
	hX = float(sin(theta) * r2) * m_fZoomWidth;
	hY = float(cos(theta) * r2) * m_fZoomHeight;

	coordinates.rfBlue[0] = hX + 0.5f;
	coordinates.rfBlue[1] = hY + 0.5f;
	coordinates.rfGreen[0] = hX + 0.5f;
	coordinates.rfGreen[1] = hY + 0.5f;
	coordinates.rfRed[0] = hX + 0.5f;
	coordinates.rfRed[1] = hY + 0.5f;
	return coordinates;//this works not very well..
}


ID3D11Texture2D* HyHMD::GetSharedTexture(HANDLE hSharedTexture)
{
	if (!hSharedTexture)
		return NULL;
	for (SharedTextures_t::iterator it = m_SharedTextureCache.begin();
		it != m_SharedTextureCache.end(); ++it)
	{
		if (it->m_hSharedTexture == hSharedTexture)
		{
			return it->m_pTexture;
		}
	}

	ID3D11Texture2D* pTexture;
	if (SUCCEEDED(pD3D11Device->OpenSharedResource(
		hSharedTexture, __uuidof(ID3D11Texture2D), (void**)&pTexture)))
	{
		SharedTextureEntry_t entry{ hSharedTexture, pTexture };
		m_SharedTextureCache.push_back(entry);
		return pTexture;
	}
	return NULL;
}//from virtual display simple



void HyHMD::Present(const PresentInfo_t* pPresentInfo, uint32_t unPresentInfoSize)
{
	ID3D11Texture2D* pTexture = GetSharedTexture((HANDLE)pPresentInfo->backbufferTextureHandle);
	IDXGIKeyedMutex* pKeyedMutex = NULL;
	HyPose eyePoses[HY_EYE_MAX];
	HyTrackingState trackInform;
	pTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&pKeyedMutex);
	m_tLastSubmitTime = clock();
	if (pKeyedMutex->AcquireSync(0, 50) != S_OK)
	{
		pKeyedMutex->Release();
		return;
	}//wait randering
	m_DispTexDesc.m_texture = pTexture;
	m_nFrameCounter = pPresentInfo->nFrameId;
	HMDDevice->GetTrackingState(HY_SUBDEV_HMD, m_nFrameCounter, trackInform);
	UpdatePose(trackInform);
	m_DispHandle->GetEyePoses(trackInform.m_pose, nullptr, eyePoses);
	HyResult rs = m_DispHandle->Submit(m_nFrameCounter, &m_DispTexDesc, 1);
	if (pKeyedMutex)
	{
		pKeyedMutex->ReleaseSync(0);
		pKeyedMutex->Release();
	}
}

void HyHMD::WaitForPresent()
{
}

bool HyHMD::GetTimeSinceLastVsync(float* pfSecondsSinceLastVsync, uint64_t* pulFrameCounter)
{
	*pfSecondsSinceLastVsync = (float)(clock() - m_tLastSubmitTime) / CLOCKS_PER_SEC;
	*pulFrameCounter = m_nFrameCounter;
	return true;
}

//private

void HyHMD::initPos()
{
	m_Pose.result = vr::TrackingResult_Running_OK;
	m_Pose.poseIsValid = true;
	m_Pose.willDriftInYaw = true;
	m_Pose.shouldApplyHeadModel = false;
	m_Pose.deviceIsConnected = true;

	m_Pose.poseTimeOffset = 0.0f;
	m_Pose.qWorldFromDriverRotation.w = 1.0;
	m_Pose.qWorldFromDriverRotation.x = 0.0;
	m_Pose.qWorldFromDriverRotation.y = 0.0;
	m_Pose.qWorldFromDriverRotation.z = 0.0;
	m_Pose.vecWorldFromDriverTranslation[0] = 0.0;
	m_Pose.vecWorldFromDriverTranslation[1] = 0.0;
	m_Pose.vecWorldFromDriverTranslation[2] = 0.0;

	m_Pose.qDriverFromHeadRotation.w = 1.0;
	m_Pose.qDriverFromHeadRotation.x = 0.0;
	m_Pose.qDriverFromHeadRotation.y = 0.0;
	m_Pose.qDriverFromHeadRotation.z = 0.0;

	m_Pose.vecDriverFromHeadTranslation[0] = 0.00f;
	m_Pose.vecDriverFromHeadTranslation[1] = -0.039f;
	m_Pose.vecDriverFromHeadTranslation[2] = -0.156f;//require further adjustment

	m_Pose.vecAcceleration[0] = 0.0;
	m_Pose.vecAcceleration[1] = 0.0;
	m_Pose.vecAcceleration[2] = 0.0;
	m_Pose.vecAngularAcceleration[0] = 0.0;
	m_Pose.vecAngularAcceleration[1] = 0.0;
	m_Pose.vecAngularAcceleration[2] = 0.0;
}

DriverPose_t HyHMD::GetPose(HyTrackingState HMDData)
{
	/*if (GetAsyncKeyState(VK_UP) != 0) {
		m_Pose.vecDriverFromHeadTranslation[2] += 0.003;
		DriverLog("vecWorldFromDriverTranslation_z:%f", m_Pose.vecDriverFromHeadTranslation[2]);
	}
	if (GetAsyncKeyState(VK_DOWN) != 0) {
		m_Pose.vecDriverFromHeadTranslation[2] -= 0.003;
		DriverLog("vecWorldFromDriverTranslation_z:%f", m_Pose.vecDriverFromHeadTranslation[2]);
	}
	if (GetAsyncKeyState(VK_LCONTROL) != 0) {
		m_Pose.vecDriverFromHeadTranslation[0] += 0.003;
		DriverLog("vecDriverFromHeadTranslation_y:%f", m_Pose.vecDriverFromHeadTranslation[0]);
	}
	if (GetAsyncKeyState(VK_LSHIFT)!= 0) {
		m_Pose.vecDriverFromHeadTranslation[0] -= 0.003;
		DriverLog("vecDriverFromHeadTranslation_y:%f", m_Pose.vecDriverFromHeadTranslation[0]);
	}
	if (GetAsyncKeyState(VK_LEFT) != 0) {
		m_Pose.vecDriverFromHeadTranslation[1] += 0.003;
		DriverLog("vecDriverFromHeadTranslation_x:%f", m_Pose.vecDriverFromHeadTranslation[1]);
	}
	if (GetAsyncKeyState(VK_RIGHT) != 0) {
		m_Pose.vecDriverFromHeadTranslation[1] -= 0.003;
		DriverLog("vecDriverFromHeadTranslation_x:%f", m_Pose.vecDriverFromHeadTranslation[1]);
	}*/

	m_Pose.result = vr::TrackingResult_Running_OK;
	m_Pose.poseIsValid = true;
	m_Pose.deviceIsConnected = true;
	m_Pose.vecPosition[0] = HMDData.m_pose.m_position.x;
	m_Pose.vecPosition[1] = HMDData.m_pose.m_position.y;
	m_Pose.vecPosition[2] = HMDData.m_pose.m_position.z;
	m_Pose.qRotation.x = HMDData.m_pose.m_rotation.x;
	m_Pose.qRotation.y = HMDData.m_pose.m_rotation.y;
	m_Pose.qRotation.z = HMDData.m_pose.m_rotation.z;
	m_Pose.qRotation.w = HMDData.m_pose.m_rotation.w;
	m_Pose.vecVelocity[0] = HMDData.m_linearVelocity.x;
	m_Pose.vecVelocity[1] = HMDData.m_linearVelocity.y;
	m_Pose.vecVelocity[2] = HMDData.m_linearVelocity.z;
	//m_Pose.vecAngularVelocity[0] = HMDData.m_angularVelocity.x;
	//m_Pose.vecAngularVelocity[1] = HMDData.m_angularVelocity.y;
	//m_Pose.vecAngularVelocity[2] = HMDData.m_angularVelocity.z;//Avoid shaking
	m_Pose.vecAngularAcceleration[0] = HMDData.m_angularAcceleration.x;
	m_Pose.vecAngularAcceleration[1] = HMDData.m_angularAcceleration.y;
	m_Pose.vecAngularAcceleration[2] = HMDData.m_angularAcceleration.z;
	m_Pose.vecAcceleration[0] = HMDData.m_linearAcceleration.x;
	m_Pose.vecAcceleration[1] = HMDData.m_linearAcceleration.y;
	m_Pose.vecAcceleration[2] = HMDData.m_linearAcceleration.z;
	if (HMDData.m_flags == HY_TRACKING_NONE) {
		m_Pose.result = vr::TrackingResult_Uninitialized;
		m_Pose.poseIsValid = false;
		m_Pose.deviceIsConnected = false;
		return m_Pose;
	}
	/*if (HMDData.m_flags == HY_TRACKING_ROTATION_TRACKED) {
		m_Pose.result = vr::TrackingResult_Fallback_RotationOnly;
		m_Pose.poseIsValid = false;
		return m_Pose;
	}*///��׷�ٲ�����
	return m_Pose;
}

/*
double* __fastcall sub_1800072C0(double* a1, float* a2)
{
	float v7; // xmm0_4
	float v15; // xmm0_4
	float v16; // xmm3_4
	float v17; // xmm1_4
	float v18; // xmm0_4
	float v19; // xmm2_4
	float v20; // xmm1_4
	float v21; // xmm3_4
	float v22; // xmm1_4
	float v23; // xmm2_4
	v7 = a2[0] + a2[5] + a2[10];
	if (v7 <= 0.0)
	{
		if (a2[0] <= a2[5] || a2[0] <= a2[10])
		{
			if (a2[5] <= a2[10])//a2[10] is max
			{
				v21 = sqrtf(a2[10] + 1.0 - a2[0] - a2[5]) * 2.0;
				v22 = a2[4] - a2[1];
				v23 = a2[8] + a2[2];
				a1[2] = (a2[9] + a2[6]) / v21;
				a1[0] = v22 / v21;
				a1[1] = v23 / v21;
				a1[3] = v21 * 0.25;
			}
			else//s2[5] is max
			{
				v18 = sqrtf(a2[5] + 1.0- a2[0] - a2[10]);
				v19 = (a2[4] + a2[1]) / (v18 * 2.0);
				a1[0] = ((a2[2] - a2[8]) / (v18 * 2.0));
				a1[1] = v19;
				v20 = a2[9] + a2[6];
				a1[2] = (v18 * 2.0) * 0.25;
				a1[3] = v20 / (v18 * 2.0);
			}
		}
		else//a2[0] is max
		{
			v15 = sqrtf(a2[0] + 1.0 - a2[5] - a2[10]);
			v16 = (a2[4] + a2[1]) / (v15 * 2.0);
			a1[0] = ((a2[9] - a2[6]) / (v15 * 2.0));
			a1[1] = ((v15 * 2.0) * 0.25);
			v17 = a2[8] + a2[2];
			a1[2] = v16;
			a1[3] = v17 / (v15 * 2.0);
		}
	}
	else//v7<0
	{
		a1[0] = (0.25 / (0.5 / sqrtf(v7 + 1.0)));
		a1[1] = (a2[9] - a2[6]) * (0.5 / v8);
		a1[2] = (a2[2] - a2[8]) * (0.5*v8);
		a1[3] = (a2[4] - a2[1]) * (0.5*v8);
	}
	return a1;
}*/ //from 00hypereal00.dll by ida, no idea for what, maybe useful