#include "StdAfx.h"
#include "InstanceBase.h"
#include "PythonTextTail.h"
#include "AbstractApplication.h"
#include "AbstractPlayer.h"
#include "PythonPlayer.h"
#include "PythonSystem.h"

#include "../EffectLib/EffectManager.h"
#include "../EffectLib/ParticleSystemData.h"
#include "../EterLib/Camera.h"

float CInstanceBase::ms_fDustGap;
float CInstanceBase::ms_fHorseDustGap;
DWORD CInstanceBase::ms_adwCRCAffectEffect[CInstanceBase::EFFECT_NUM];
std::string CInstanceBase::ms_astAffectEffectAttachBone[EFFECT_NUM];

#define BYTE_COLOR_TO_D3DX_COLOR(r, g, b) D3DXCOLOR(float(r)/255.0f, float(g)/255.0f, float(b)/255.0f, 1.0f)

/*
D3DXCOLOR CInstanceBase::ms_kD3DXClrPC(0xFFFFD84D);//1.0f, 0.8470f, 0.3f, 1.0f
D3DXCOLOR CInstanceBase::ms_kD3DXClrNPC(0xFF7BE85E);//0.4823f, 0.9098f, 0.3686f, 1.0f
D3DXCOLOR CInstanceBase::ms_kD3DXClrMOB(0xFFEC170a);//0.9254f, 0.0901f, 0.0392f, 1.0f
D3DXCOLOR CInstanceBase::ms_kD3DXClrPVP(0xFF8532D9);
D3DXCOLOR CInstanceBase::ms_kD3DXClrPVPSelf(0xFFEE36DF);
D3DXCOLOR CInstanceBase::ms_kD3DXClrKiller = BYTE_COLOR_TO_D3DX_COLOR(180, 100, 0);
D3DXCOLOR CInstanceBase::ms_kD3DXClrTitle[CInstanceBase::TITLE_MAX_NUM] =
{
	BYTE_COLOR_TO_D3DX_COLOR(  0, 204, 255),
	BYTE_COLOR_TO_D3DX_COLOR(  0, 144, 255),
	BYTE_COLOR_TO_D3DX_COLOR( 92, 110, 255),
	BYTE_COLOR_TO_D3DX_COLOR(155, 155, 255),
	0xFFFFFFFF, // None
	BYTE_COLOR_TO_D3DX_COLOR(207, 117,   0),
	BYTE_COLOR_TO_D3DX_COLOR(235,  83,   0),
	BYTE_COLOR_TO_D3DX_COLOR(227,   0,   0),
	BYTE_COLOR_TO_D3DX_COLOR(255,   0,   0),
};
*/

D3DXCOLOR g_akD3DXClrTitle[CInstanceBase::TITLE_NUM];
D3DXCOLOR g_akD3DXClrName[CInstanceBase::NAMECOLOR_NUM];

std::map<int, std::string> g_TitleNameMap;
std::set<DWORD> g_kSet_dwPVPReadyKey;
std::set<DWORD> g_kSet_dwPVPKey;
std::set<DWORD> g_kSet_dwGVGKey;
std::set<DWORD> g_kSet_dwDUELKey;

bool g_isEmpireNameMode=false;

void  CInstanceBase::SetEmpireNameMode(bool isEnable)
{
	g_isEmpireNameMode=isEnable;

	if (isEnable)
	{
		g_akD3DXClrName[NAMECOLOR_MOB]=g_akD3DXClrName[NAMECOLOR_EMPIRE_MOB];
		g_akD3DXClrName[NAMECOLOR_NPC]=g_akD3DXClrName[NAMECOLOR_EMPIRE_NPC];
		g_akD3DXClrName[NAMECOLOR_PC]=g_akD3DXClrName[NAMECOLOR_NORMAL_PC];

		for (UINT uEmpire=1; uEmpire<EMPIRE_NUM; ++uEmpire)
			g_akD3DXClrName[NAMECOLOR_PC+uEmpire]=g_akD3DXClrName[NAMECOLOR_EMPIRE_PC+uEmpire];
		
	}
	else
	{
		g_akD3DXClrName[NAMECOLOR_MOB]=g_akD3DXClrName[NAMECOLOR_NORMAL_MOB];
		g_akD3DXClrName[NAMECOLOR_NPC]=g_akD3DXClrName[NAMECOLOR_NORMAL_NPC];

		for (UINT uEmpire=0; uEmpire<EMPIRE_NUM; ++uEmpire)
			g_akD3DXClrName[NAMECOLOR_PC+uEmpire]=g_akD3DXClrName[NAMECOLOR_NORMAL_PC];
	}
}

const D3DXCOLOR& CInstanceBase::GetIndexedNameColor(UINT eNameColor)
{
	if (eNameColor>=NAMECOLOR_NUM)
	{
		static D3DXCOLOR s_kD3DXClrNameDefault(0xffffffff);
		return s_kD3DXClrNameDefault;
	}

	return g_akD3DXClrName[eNameColor];
}

void CInstanceBase::AddDamageEffect(DWORD damage, BYTE flag, bool bSelf, bool bTarget)
{
    if (CPythonSystem::Instance().IsShowDamage())
    {
        SEffectDamage sDamage;
        sDamage.bSelf = bSelf;
        sDamage.bTarget = bTarget;
        sDamage.damage = damage;
        sDamage.flag = flag;
        m_DamageQueue.push(sDamage);
    }
}

bool CInstanceBase::ProcessDamage()
{
	if (m_DamageQueue.empty())
	{
		return false;
	}

	SEffectDamage sDamage = m_DamageQueue.front();

	m_DamageQueue.pop();

	DWORD damage = sDamage.damage;
	BYTE flag = sDamage.flag;
	BOOL bSelf = sDamage.bSelf;
	BOOL bTarget = sDamage.bTarget;

	CCamera * pCamera = CCameraManager::Instance().GetCurrentCamera();	
	float cameraAngle = GetDegreeFromPosition2(pCamera->GetTarget().x,pCamera->GetTarget().y,pCamera->GetEye().x,pCamera->GetEye().y);

	DWORD FONT_WIDTH = 30;
	
	CEffectManager& rkEftMgr=CEffectManager::Instance();

	D3DXVECTOR3 v3Pos = m_GraphicThingInstance.GetPosition();
	v3Pos.z += float(m_GraphicThingInstance.GetHeight());

	D3DXVECTOR3 v3Rot = D3DXVECTOR3(0.0f, 0.0f, cameraAngle);

	if ( (flag & DAMAGE_DODGE) || (flag & DAMAGE_BLOCK) )
	{
		if(bSelf)
			rkEftMgr.CreateEffect(ms_adwCRCAffectEffect[EFFECT_DAMAGE_MISS],v3Pos,v3Rot);
		else
			rkEftMgr.CreateEffect(ms_adwCRCAffectEffect[EFFECT_DAMAGE_TARGETMISS],v3Pos,v3Rot);
		return true;
	}
	else if (flag & DAMAGE_CRITICAL)
	{
	}

	std::string strDamageType;
	DWORD rdwCRCEft = 0;
	{
		if(bSelf)
		{
			strDamageType = "damage_";
			if(m_bDamageEffectType==0)
				rdwCRCEft = EFFECT_DAMAGE_SELFDAMAGE;
			else
				rdwCRCEft = EFFECT_DAMAGE_SELFDAMAGE2;
			m_bDamageEffectType = !m_bDamageEffectType;
		}
		else if(bTarget == false)
		{
			strDamageType = "nontarget_";
			rdwCRCEft = EFFECT_DAMAGE_NOT_TARGET;
			return true;//ÇöŔç Ŕűżë ľČµĘ.
		}
		else
		{
			strDamageType = "target_";
			rdwCRCEft = EFFECT_DAMAGE_TARGET;
		}
	}
	
	DWORD index = 0;
	DWORD num = 0;
	std::vector<std::string> textures;
	while(damage>0)
	{
		if(index > 7)
		{
			TraceError("ProcessDamageą«ÇŃ·çÇÁ °ˇ´ÉĽş");
			return false;
		}
		num = damage%10;
		damage /= 10;
		char numBuf[MAX_PATH];
		sprintf(numBuf,"%d.dds",num);
		textures.push_back("d:/ymir work/effect/affect/damagevalue/"+strDamageType+numBuf);
		
		rkEftMgr.SetEffectTextures(ms_adwCRCAffectEffect[rdwCRCEft],textures);
		
		D3DXMATRIX matrix,matTrans;
		D3DXMatrixIdentity(&matrix);
		matrix._41 = v3Pos.x;
		matrix._42 = v3Pos.y;
		matrix._43 = v3Pos.z;
		D3DXMatrixTranslation(&matrix,v3Pos.x,v3Pos.y,v3Pos.z);
		D3DXMatrixMultiply(&matrix,&pCamera->GetInverseViewMatrix(),&matrix);
		D3DXMatrixTranslation(&matTrans,FONT_WIDTH*index,0,0);
		matTrans._41 = -matTrans._41;
		matrix = matTrans*matrix;
		D3DXMatrixMultiply(&matrix,&pCamera->GetViewMatrix(),&matrix);
		
		rkEftMgr.CreateEffect(ms_adwCRCAffectEffect[rdwCRCEft],D3DXVECTOR3(matrix._41,matrix._42,matrix._43)
			,v3Rot);	
		
		textures.clear();

		index++;
	}	
	return true;
}

void CInstanceBase::AttachSpecialEffect(DWORD effect)
{
	__AttachEffect(effect);
}

void CInstanceBase::LevelUp()
{
	__AttachEffect(EFFECT_LEVELUP);
}

void CInstanceBase::SkillUp()
{
	__AttachEffect(EFFECT_SKILLUP);
}

void CInstanceBase::CreateSpecialEffect(DWORD iEffectIndex)
{
	const D3DXMATRIX & c_rmatGlobal = m_GraphicThingInstance.GetTransform();

	DWORD dwEffectIndex = CEffectManager::Instance().GetEmptyIndex();
	DWORD dwEffectCRC = ms_adwCRCAffectEffect[iEffectIndex];
	CEffectManager::Instance().CreateEffectInstance(dwEffectIndex, dwEffectCRC);
	CEffectManager::Instance().SelectEffectInstance(dwEffectIndex);
	CEffectManager::Instance().SetEffectInstanceGlobalMatrix(c_rmatGlobal);
}

void CInstanceBase::__EffectContainer_Destroy()
{
	SEffectContainer::Dict& rkDctEftID=__EffectContainer_GetDict();

	SEffectContainer::Dict::iterator i;
	for (i=rkDctEftID.begin(); i!=rkDctEftID.end(); ++i)
		__DetachEffect(i->second);

	rkDctEftID.clear();
}

void CInstanceBase::__EffectContainer_Initialize()
{
	SEffectContainer::Dict& rkDctEftID=__EffectContainer_GetDict();
	rkDctEftID.clear();
}

CInstanceBase::SEffectContainer::Dict& CInstanceBase::__EffectContainer_GetDict()
{
	return m_kEffectContainer.m_kDct_dwEftID;
}

DWORD CInstanceBase::__EffectContainer_AttachEffect(DWORD dwEftKey)
{
	SEffectContainer::Dict& rkDctEftID = __EffectContainer_GetDict();
	SEffectContainer::Dict::iterator f = rkDctEftID.find(dwEftKey);
	if (rkDctEftID.end() != f)
		return 0;

	DWORD dwEftID = __AttachEffect(dwEftKey);
	rkDctEftID.insert(SEffectContainer::Dict::value_type(dwEftKey, dwEftID));
	return dwEftID;
}

void CInstanceBase::__EffectContainer_DetachEffect(DWORD dwEftKey)
{
	SEffectContainer::Dict& rkDctEftID = __EffectContainer_GetDict();
	SEffectContainer::Dict::iterator f = rkDctEftID.find(dwEftKey);
	if (rkDctEftID.end() == f)
		return;

	__DetachEffect(f->second);

	rkDctEftID.erase(f);
}


void CInstanceBase::__AttachEmpireEffect(DWORD eEmpire)
{
	if (!__IsExistMainInstance())
		return;

	CInstanceBase* pkInstMain=__GetMainInstancePtr();

	if (IsWarp())
		return;
	if (IsObject())
		return;
	if (IsFlag())
		return;
	if (IsResource())
		return;

	if (pkInstMain->IsGameMaster())
	{
	}
	else
	{
		if (pkInstMain->IsSameEmpire(*this))
			return;

		// HIDE_OTHER_EMPIRE_EUNHYEONG_ASSASSIN
		if (IsAffect(AFFECT_EUNHYEONG))
			return;
		// END_OF_HIDE_OTHER_EMPIRE_EUNHYEONG_ASSASSIN
	}

	if (IsGameMaster())
		return;

	__EffectContainer_AttachEffect(EFFECT_EMPIRE+eEmpire);
}

void CInstanceBase::__AttachSelectEffect()
{
	__EffectContainer_AttachEffect(EFFECT_SELECT);
}

void CInstanceBase::__DetachSelectEffect()
{
	__EffectContainer_DetachEffect(EFFECT_SELECT);
}

void CInstanceBase::__AttachTargetEffect()
{
	__EffectContainer_AttachEffect(EFFECT_TARGET);
}

void CInstanceBase::__DetachTargetEffect()
{
	__EffectContainer_DetachEffect(EFFECT_TARGET);
}

void CInstanceBase::__StoneSmoke_Inialize()
{
	m_kStoneSmoke.m_dwEftID=0;
}

void CInstanceBase::__StoneSmoke_Destroy()
{
	if (!m_kStoneSmoke.m_dwEftID)
		return;

	__DetachEffect(m_kStoneSmoke.m_dwEftID);
	m_kStoneSmoke.m_dwEftID=0;
}

void CInstanceBase::__StoneSmoke_Create(DWORD eSmoke)
{
	m_kStoneSmoke.m_dwEftID=m_GraphicThingInstance.AttachSmokeEffect(eSmoke);
}

void CInstanceBase::SetAlpha(float fAlpha)
{
	__SetBlendRenderingMode();
	__SetAlphaValue(fAlpha);
}

bool CInstanceBase::UpdateDeleting()
{
	Update();
	Transform();

	IAbstractApplication& rApp=IAbstractApplication::GetSingleton();

	float fAlpha = __GetAlphaValue() - (rApp.GetGlobalElapsedTime() * 1.5f);
	__SetAlphaValue(fAlpha);

	if (fAlpha < 0.0f)
		return false;

	return true;
}

void CInstanceBase::DeleteBlendOut()
{
	__SetBlendRenderingMode();
	__SetAlphaValue(1.0f);
	DetachTextTail();

	IAbstractPlayer& rkPlayer=IAbstractPlayer::GetSingleton();
	rkPlayer.NotifyDeletingCharacterInstance(GetVirtualID());
}

void CInstanceBase::ClearPVPKeySystem()
{
	g_kSet_dwPVPReadyKey.clear();
	g_kSet_dwPVPKey.clear();
	g_kSet_dwGVGKey.clear();
	g_kSet_dwDUELKey.clear();
}

void CInstanceBase::InsertPVPKey(DWORD dwVIDSrc, DWORD dwVIDDst)
{
	DWORD dwPVPKey=__GetPVPKey(dwVIDSrc, dwVIDDst);

	g_kSet_dwPVPKey.insert(dwPVPKey);
}

void CInstanceBase::InsertPVPReadyKey(DWORD dwVIDSrc, DWORD dwVIDDst)
{
	DWORD dwPVPReadyKey=__GetPVPKey(dwVIDSrc, dwVIDDst);

	g_kSet_dwPVPKey.insert(dwPVPReadyKey);
}

void CInstanceBase::RemovePVPKey(DWORD dwVIDSrc, DWORD dwVIDDst)
{
	DWORD dwPVPKey=__GetPVPKey(dwVIDSrc, dwVIDDst);

	g_kSet_dwPVPKey.erase(dwPVPKey);
}

void CInstanceBase::InsertGVGKey(DWORD dwSrcGuildVID, DWORD dwDstGuildVID)
{
	DWORD dwGVGKey = __GetPVPKey(dwSrcGuildVID, dwDstGuildVID);
	g_kSet_dwGVGKey.insert(dwGVGKey);
}

void CInstanceBase::RemoveGVGKey(DWORD dwSrcGuildVID, DWORD dwDstGuildVID)
{
	DWORD dwGVGKey = __GetPVPKey(dwSrcGuildVID, dwDstGuildVID);
	g_kSet_dwGVGKey.erase(dwGVGKey);
}

void CInstanceBase::InsertDUELKey(DWORD dwVIDSrc, DWORD dwVIDDst)
{
	DWORD dwPVPKey=__GetPVPKey(dwVIDSrc, dwVIDDst);

	g_kSet_dwDUELKey.insert(dwPVPKey);
}

DWORD CInstanceBase::__GetPVPKey(DWORD dwVIDSrc, DWORD dwVIDDst)
{
	if (dwVIDSrc>dwVIDDst)
		std::swap(dwVIDSrc, dwVIDDst);

	DWORD awSrc[2];
	awSrc[0]=dwVIDSrc;
	awSrc[1]=dwVIDDst;

    const BYTE * s = (const BYTE *) awSrc;
    const BYTE * end = s + sizeof(awSrc);
    unsigned long h = 0;

    while (s < end)
    {
        h *= 16777619;
        h ^= (BYTE) *(BYTE *) (s++);
    }

    return h;
}

bool CInstanceBase::__FindPVPKey(DWORD dwVIDSrc, DWORD dwVIDDst)
{
	DWORD dwPVPKey=__GetPVPKey(dwVIDSrc, dwVIDDst);

	if (g_kSet_dwPVPKey.end()==g_kSet_dwPVPKey.find(dwPVPKey))
		return false;

	return true;
}

bool CInstanceBase::__FindPVPReadyKey(DWORD dwVIDSrc, DWORD dwVIDDst)
{
	DWORD dwPVPKey=__GetPVPKey(dwVIDSrc, dwVIDDst);

	if (g_kSet_dwPVPReadyKey.end()==g_kSet_dwPVPReadyKey.find(dwPVPKey))
		return false;

	return true;
}
//±ćµĺŔü˝Ă »ó´ë ±ćµĺŔÎÁö Č®ŔÎÇŇ¶§.
bool CInstanceBase::__FindGVGKey(DWORD dwSrcGuildID, DWORD dwDstGuildID)
{
	DWORD dwGVGKey=__GetPVPKey(dwSrcGuildID, dwDstGuildID);

	if (g_kSet_dwGVGKey.end()==g_kSet_dwGVGKey.find(dwGVGKey))
		return false;

	return true;
}
//´ë·Ă ¸đµĺżˇĽ­´Â ´ë·Ă »ó´ë¸¸ °ř°ÝÇŇ Ľö ŔÖ´Ů.
bool CInstanceBase::__FindDUELKey(DWORD dwVIDSrc, DWORD dwVIDDst)
{
	DWORD dwDUELKey=__GetPVPKey(dwVIDSrc, dwVIDDst);

	if (g_kSet_dwDUELKey.end()==g_kSet_dwDUELKey.find(dwDUELKey))
		return false;

	return true;
}

bool CInstanceBase::IsPVPInstance(CInstanceBase& rkInstSel)
{
	DWORD dwVIDSrc=GetVirtualID();
	DWORD dwVIDDst=rkInstSel.GetVirtualID();

	DWORD dwGuildIDSrc=GetGuildID();
	DWORD dwGuildIDDst=rkInstSel.GetGuildID();

	if (GetDuelMode())	//´ë·Ă ¸đµĺŔĎ¶§´Â ~_~
		return true;	

	return __FindPVPKey(dwVIDSrc, dwVIDDst) || __FindGVGKey(dwGuildIDSrc, dwGuildIDDst);
											//__FindDUELKey(dwVIDSrc, dwVIDDst);
}

const D3DXCOLOR& CInstanceBase::GetNameColor()
{
	return GetIndexedNameColor(GetNameColorIndex());
}

uint32_t CInstanceBase::GetNameColorIndex()
{
	if (IsPC())
	{
		if (m_isKiller)
		{
			return NAMECOLOR_PK;
		}

		if (__IsExistMainInstance() && !__IsMainInstance())
		{
			CInstanceBase* pkInstMain = __GetMainInstancePtr();
			if (!pkInstMain)
			{
				TraceError("CInstanceBase::GetNameColorIndex - MainInstance is NULL");
				return NAMECOLOR_PC;
			}

			auto dwVIDMain = pkInstMain->GetVirtualID();
			auto dwVIDSelf = GetVirtualID();

			if (pkInstMain->GetDuelMode())
			{
				switch (pkInstMain->GetDuelMode())
				{
					case DUEL_CANNOTATTACK:
						return NAMECOLOR_PC + GetEmpireID();
					case DUEL_START:
						if (__FindDUELKey(dwVIDMain, dwVIDSelf))
							return NAMECOLOR_PVP;
						else
							return NAMECOLOR_PC + GetEmpireID();
				}
			}

			if (pkInstMain->IsSameEmpire(*this))
			{
				if (__FindPVPKey(dwVIDMain, dwVIDSelf))
				{
					return NAMECOLOR_PVP;
				}

				auto dwGuildIDMain = pkInstMain->GetGuildID();
				auto dwGuildIDSelf = GetGuildID();
				if (__FindGVGKey(dwGuildIDMain, dwGuildIDSelf))
				{
					return NAMECOLOR_PVP;
				}
			}
			else
			{
				return NAMECOLOR_PVP;
			}
		}

		IAbstractPlayer& rPlayer=IAbstractPlayer::GetSingleton();
		if (rPlayer.IsPartyMemberByVID(GetVirtualID()))
			return NAMECOLOR_PARTY;

		return NAMECOLOR_PC + GetEmpireID();
		
	}
	else if (IsNPC())
	{
		return NAMECOLOR_NPC;
	}
	else if (IsEnemy())
	{
		return NAMECOLOR_MOB;
	}
	else if (IsPoly())
	{
		return NAMECOLOR_MOB;
	}


	return D3DXCOLOR(0xffffffff);
}

const D3DXCOLOR& CInstanceBase::GetTitleColor()
{
	UINT uGrade = GetAlignmentGrade();
	if ( uGrade >= TITLE_NUM)
	{
		static D3DXCOLOR s_kD3DXClrTitleDefault(0xffffffff);
		return s_kD3DXClrTitleDefault;
	}

	return g_akD3DXClrTitle[uGrade];
}

void CInstanceBase::AttachTextTail()
{
	// Declare and initialize fTextTailHeight and s_kD3DXClrTextTail outside of the function.
	// For example:
	// static float fTextTailHeight = 10.0f;
	// static D3DXCOLOR s_kD3DXClrTextTail = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);

	DWORD dwVID = GetVirtualID();

	if (m_isTextTail)
	{
		TraceError("CInstanceBase::AttachTextTail - VID [%d] ALREADY EXIST", dwVID);
		return;
	}

	m_isTextTail = true;
	float fTextTailHeight;

	if (IsMountingHorse())
	{
		fTextTailHeight = 110.0f;
	}
	else
	{
		fTextTailHeight = 10.0f;
	}

	// Declare and initialize the textTailInfo map outside of the function
	// For example:
	// std::unordered_map<DWORD, TextTailInfo> textTailInfo;
	struct TextTailInfo
	{
		DWORD guildID;
		D3DXCOLOR color;
		float height;
	};

	std::unordered_map<DWORD, TextTailInfo> textTailInfo;

	// Use a local variable to store the result of CPythonTextTail::Instance()__RunNextCombo(wComboType=0, wComboIndex=1) - m_pkCurRaceData->GetComboDataPointer(m_wcurMotionMode=6, &pComboData) == NULL
	auto& pythonTextTail = CPythonTextTail::Instance();

	// Check if the character's text tail information is already in the textTailInfo map
	auto it = textTailInfo.find(dwVID);
	if (it == textTailInfo.end())
	{
		D3DXCOLOR s_kD3DXClrTextTail;
		s_kD3DXClrTextTail = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);

		TextTailInfo info;
		info.guildID = m_dwGuildID;
		info.color = s_kD3DXClrTextTail;
		info.height = fTextTailHeight;

		textTailInfo[dwVID] = info;

		pythonTextTail.RegisterCharacterTextTail(info.guildID, dwVID, info.color, info.height);
	}
	else
	{
		pythonTextTail.RegisterCharacterTextTail(it->second.guildID, dwVID, it->second.color, it->second.height);
	}

	// CHARACTER_LEVEL
	if (m_dwLevel)
	{
		UpdateTextTailLevel(m_dwLevel);
	}
}

void CInstanceBase::DetachTextTail()
{
	if (!m_isTextTail)
		return;

	m_isTextTail=false;
	CPythonTextTail::Instance().DeleteCharacterTextTail(GetVirtualID());
}

void CInstanceBase::UpdateTextTailLevel(uint32_t level)
{
	static const D3DXCOLOR kLevelColor = D3DXCOLOR(152.0f / 255.0f, 255.0f / 255.0f, 51.0f / 255.0f, 1.0f);
	m_dwLevel = level;

	std::string szText;

	if (IsGameMaster())
	{
		szText = "[Mistrz Gry] ";
	}
	else
	{
		szText = "Poz. " + std::to_string(level) + " ";
	}

	CPythonTextTail::Instance().AttachLevel(GetVirtualID(), szText.c_str(), kLevelColor);
}

void CInstanceBase::RefreshTextTail()
{
	// Use a local variable to store the result of CPythonTextTail::Instance()
	auto& pythonTextTail = CPythonTextTail::Instance();

	pythonTextTail.SetCharacterTextTailColor(GetVirtualID(), GetNameColor());

	int iAlignmentGrade = GetAlignmentGrade();
	if (iAlignmentGrade != TITLE_NONE)
	{
		auto itor = g_TitleNameMap.find(iAlignmentGrade);
		if (itor != g_TitleNameMap.end())
		{
			const std::string& c_rstrTitleName = itor->second;
			pythonTextTail.AttachTitle(GetVirtualID(), c_rstrTitleName.c_str(), GetTitleColor());
		}
	}
	else
	{
		pythonTextTail.DetachTitle(GetVirtualID());
	}
}


void CInstanceBase::RefreshTextTailTitle()
{
	RefreshTextTail();
}

// 2004.07.25.myevan.ŔĚĆĺĆ® ľČ şŮ´Â ą®Á¦ ÇŘ°á
/////////////////////////////////////////////////
void CInstanceBase::__ClearAffectFlagContainer()
{
	m_kAffectFlagContainer.Clear();
}

void CInstanceBase::__ClearAffects()
{
	if (IsStone())
	{
		__StoneSmoke_Destroy();
	}
	else
	{
		for (int iAffect=0; iAffect<AFFECT_NUM; ++iAffect)
		{
			__DetachEffect(m_adwCRCAffectEffect[iAffect]);
			m_adwCRCAffectEffect[iAffect]=0;
		}

		__ClearAffectFlagContainer();
	}

	m_GraphicThingInstance.__OnClearAffects();
}

/////////////////////////////////////////////////

void CInstanceBase::__SetNormalAffectFlagContainer(const CAffectFlagContainer& c_rkAffectFlagContainer)
{
	for (int i = 0; i < CAffectFlagContainer::BIT_SIZE; ++i)
	{
		bool isOldSet = m_kAffectFlagContainer.IsSet(i);
		bool isNewSet = c_rkAffectFlagContainer.IsSet(i);

		if (isOldSet != isNewSet)
		{
			__SetAffect(i, isNewSet);

			if (isNewSet)
				m_GraphicThingInstance.__OnSetAffect(i);
			else
				m_GraphicThingInstance.__OnResetAffect(i);
		}
	}

	m_kAffectFlagContainer.CopyInstance(c_rkAffectFlagContainer);
}

void CInstanceBase::__SetStoneSmokeFlagContainer(const CAffectFlagContainer& c_rkAffectFlagContainer)
{
	m_kAffectFlagContainer.CopyInstance(c_rkAffectFlagContainer);

	DWORD eSmoke;
	if (m_kAffectFlagContainer.IsSet(STONE_SMOKE8))
		eSmoke=3;
	else if (m_kAffectFlagContainer.IsSet(STONE_SMOKE5)|m_kAffectFlagContainer.IsSet(STONE_SMOKE6)|m_kAffectFlagContainer.IsSet(STONE_SMOKE7))
		eSmoke=2;
	else if (m_kAffectFlagContainer.IsSet(STONE_SMOKE2)|m_kAffectFlagContainer.IsSet(STONE_SMOKE3)|m_kAffectFlagContainer.IsSet(STONE_SMOKE4))
		eSmoke=1;
	else
		eSmoke=0;

	__StoneSmoke_Destroy();
	__StoneSmoke_Create(eSmoke);
}

void CInstanceBase::SetAffectFlagContainer(const CAffectFlagContainer& c_rkAffectFlagContainer)
{
	if (IsBuilding())
	{
		return;		
	}
	else if (IsStone())
	{
		__SetStoneSmokeFlagContainer(c_rkAffectFlagContainer);
	}
	else
	{
		__SetNormalAffectFlagContainer(c_rkAffectFlagContainer);
		UpdateAttachingEffectVisibility(c_rkAffectFlagContainer);
	}
}

void CInstanceBase::UpdateAttachingEffectVisibility(const CAffectFlagContainer& c_rkAffectFlagContainer)
{
	if (c_rkAffectFlagContainer.IsSet(AFFECT_REVIVE_INVISIBILITY) || c_rkAffectFlagContainer.IsSet(AFFECT_EUNHYEONG))
	{
		if (__IsMainInstance())
		{
			m_GraphicThingInstance.ShowAllAttachingEffect();
		}
		else
		{
			m_GraphicThingInstance.HideAllAttachingEffect();
		}
	}
}

void CInstanceBase::SCRIPT_SetAffect(UINT eAffect, bool isVisible)
{
	__SetAffect(eAffect, isVisible);
}

void CInstanceBase::__SetReviveInvisibilityAffect(bool isVisible)
{
	if (isVisible)
	{
		if (IsWearingDress())
		{
			return;
		}

		if (__IsMainInstance() || __MainCanSeeHiddenThing())
		{
			m_GraphicThingInstance.BlendAlphaValue(0.5f, 1.0f);
		}
		else
		{
			m_GraphicThingInstance.BlendAlphaValue(0.0f, 1.0f);

		}
	}
	else
	{
		if (!IsAffect(AFFECT_EUNHYEONG))
		{
			m_GraphicThingInstance.BlendAlphaValue(1.0f, 1.0f);
			m_GraphicThingInstance.ShowAllAttachingEffect();
		}
	}
}

void CInstanceBase::__Assassin_SetEunhyeongAffect(bool isVisible)
{
	if (isVisible)
	{
		if (IsWearingDress())
		{
			return;
		}

		if (__IsMainInstance() || __MainCanSeeHiddenThing())
		{
			m_GraphicThingInstance.BlendAlphaValue(0.5f, 1.0f);
		}
		else
		{
			m_GraphicThingInstance.BlendAlphaValue(0.0f, 1.0f);
			m_GraphicThingInstance.HideAllAttachingEffect();
		}
	}
	else
	{

		m_GraphicThingInstance.BlendAlphaValue(1.0f, 1.0f);
		m_GraphicThingInstance.ShowAllAttachingEffect();
	}
}

void CInstanceBase::__Shaman_SetParalysis(bool isParalysis)
{
	m_GraphicThingInstance.SetParalysis(isParalysis);
}

void CInstanceBase::__Warrior_SetGeomgyeongAffect(bool isVisible)
{
	if (isVisible)
	{
		if (IsWearingDress())
			return;

		if (m_kWarrior.m_dwGeomgyeongEffect)
			__DetachEffect(m_kWarrior.m_dwGeomgyeongEffect);

		m_GraphicThingInstance.SetReachScale(1.5f);
		if (m_GraphicThingInstance.IsTwoHandMode())
			m_kWarrior.m_dwGeomgyeongEffect=__AttachEffect(EFFECT_WEAPON+WEAPON_TWOHAND);
		else
			m_kWarrior.m_dwGeomgyeongEffect=__AttachEffect(EFFECT_WEAPON+WEAPON_ONEHAND);
	}
	else
	{
		m_GraphicThingInstance.SetReachScale(1.0f);

		__DetachEffect(m_kWarrior.m_dwGeomgyeongEffect);
		m_kWarrior.m_dwGeomgyeongEffect=0;
	}
}

void CInstanceBase::__SetAffect(UINT eAffect, bool isVisible)
{
	switch (eAffect)
	{
		case AFFECT_YMIR:
			if (IsAffect(AFFECT_INVISIBILITY))
				return;
			break;

		case AFFECT_CHEONGEUN:
			m_GraphicThingInstance.SetResistFallen(isVisible);
			break;
		case AFFECT_GEOMGYEONG:
			__Warrior_SetGeomgyeongAffect(isVisible);
			return;
			break;
		case AFFECT_REVIVE_INVISIBILITY:
			__SetReviveInvisibilityAffect(isVisible);
			break;
		case AFFECT_EUNHYEONG:
			__Assassin_SetEunhyeongAffect(isVisible);
			break;
		case AFFECT_GYEONGGONG:
		case AFFECT_KWAESOK:
			if (isVisible)
				if (!IsWalking())
					return;
			break;

		case AFFECT_INVISIBILITY:
		{
			if (isVisible)
			{
				m_GraphicThingInstance.HideAllAttachingEffect();
				__EffectContainer_Destroy();
				DetachTextTail();
			}
			else
			{
				m_GraphicThingInstance.ShowAllAttachingEffect();
				m_GraphicThingInstance.BlendAlphaValue(1.0f, 1.0f);
				AttachTextTail();
				RefreshTextTail();
			}
		}
		return;
		break;

		case AFFECT_STUN:
			m_GraphicThingInstance.SetSleep(isVisible);
			break;
	}

	if (eAffect>=AFFECT_NUM)
	{
		TraceError("CInstanceBase[VID:%d]::SetAffect(eAffect:%d<AFFECT_NUM:%d, isVisible=%d)", GetVirtualID(), eAffect, isVisible);
		return;
	}

	if (isVisible)
	{
		if (!m_adwCRCAffectEffect[eAffect])
		{
			m_adwCRCAffectEffect[eAffect]=__AttachEffect(EFFECT_AFFECT+eAffect);
		}
	}
	else
	{
		if (m_adwCRCAffectEffect[eAffect])
		{
			__DetachEffect(m_adwCRCAffectEffect[eAffect]);
			m_adwCRCAffectEffect[eAffect]=0;
		}
	}
}

bool CInstanceBase::IsPossibleEmoticon()
{
	CEffectManager& rkEftMgr=CEffectManager::Instance();
	for(DWORD eEmoticon = 0; eEmoticon < EMOTICON_NUM; eEmoticon++)
	{
		DWORD effectID = ms_adwCRCAffectEffect[EFFECT_EMOTICON+eEmoticon];
		if( effectID &&	rkEftMgr.IsAliveEffect(effectID) )
			return false;
	}

	if(ELTimer_GetMSec() - m_dwEmoticonTime < 1000)
	{
		TraceError("ELTimer_GetMSec() - m_dwEmoticonTime");
		return false;
	}

	return true;
}

void CInstanceBase::SetFishEmoticon()
{
	SetEmoticon(EMOTICON_FISH);
}

void CInstanceBase::SetEmoticon(UINT eEmoticon)
{
	if (eEmoticon>=EMOTICON_NUM)
	{
		TraceError("CInstanceBase[VID:%d]::SetEmoticon(eEmoticon:%d<EMOTICON_NUM:%d, isVisible=%d)",
			GetVirtualID(), eEmoticon);
		return;
	}
	if (IsPossibleEmoticon())
	{
		D3DXVECTOR3 v3Pos = m_GraphicThingInstance.GetPosition();
		v3Pos.z += float(m_GraphicThingInstance.GetHeight());

		//CEffectManager& rkEftMgr=CEffectManager::Instance();
		CCamera * pCamera = CCameraManager::Instance().GetCurrentCamera();
		
		D3DXVECTOR3 v3Dir = (pCamera->GetEye()-v3Pos)*9/10;	
		v3Pos = pCamera->GetEye()-v3Dir;

		v3Pos = D3DXVECTOR3(0,0,0);
		v3Pos.z += float(m_GraphicThingInstance.GetHeight());

		//rkEftMgr.CreateEffect(ms_adwCRCAffectEffect[EFFECT_EMOTICON+eEmoticon],v3Pos,D3DXVECTOR3(0,0,0));
		m_GraphicThingInstance.AttachEffectByID(0, NULL, ms_adwCRCAffectEffect[EFFECT_EMOTICON+eEmoticon],&v3Pos);
		m_dwEmoticonTime = ELTimer_GetMSec();
	}
}

void CInstanceBase::SetDustGap(float fDustGap)
{
	ms_fDustGap=fDustGap;
}

void CInstanceBase::SetHorseDustGap(float fDustGap)
{
	ms_fHorseDustGap=fDustGap;
}

void CInstanceBase::__DetachEffect(DWORD dwEID)
{
	m_GraphicThingInstance.DettachEffect(dwEID);
}

DWORD CInstanceBase::__AttachEffect(UINT eEftType)
{
	// 2004.07.17.levites.isShow¸¦ ViewFrustumCheck·Î şŻ°ć
	if (IsAffect(AFFECT_INVISIBILITY))
		return 0;

	if (eEftType>=EFFECT_NUM)
		return 0;

	if (ms_astAffectEffectAttachBone[eEftType].empty())
	{
		return m_GraphicThingInstance.AttachEffectByID(0, NULL, ms_adwCRCAffectEffect[eEftType]);
	}
	else
	{
		std::string & rstrBoneName = ms_astAffectEffectAttachBone[eEftType];
		const char * c_szBoneName;
		// ľçĽŐżˇ şŮŔĎ ¶§ »çżëÇŃ´Ů.
		// ŔĚ·± ˝ÄŔÇ żążÜ Ăł¸®¸¦ ÇŘłőŔş °ÍŔş Äł¸ŻĹÍ ¸¶´Ů Equip ŔÇ Bone Name ŔĚ ´Ů¸Ł±â ¶§ą®.
		if (0 == rstrBoneName.compare("PART_WEAPON"))
		{
			if (m_GraphicThingInstance.GetAttachingBoneName(CRaceData::PART_WEAPON, &c_szBoneName))
			{
				return m_GraphicThingInstance.AttachEffectByID(0, c_szBoneName, ms_adwCRCAffectEffect[eEftType]);
			}
		}
		else if (0 == rstrBoneName.compare("PART_WEAPON_LEFT"))
		{
			if (m_GraphicThingInstance.GetAttachingBoneName(CRaceData::PART_WEAPON_LEFT, &c_szBoneName))
			{
				return m_GraphicThingInstance.AttachEffectByID(0, c_szBoneName, ms_adwCRCAffectEffect[eEftType]);
			}
		}
		else
		{
			return m_GraphicThingInstance.AttachEffectByID(0, rstrBoneName.c_str(), ms_adwCRCAffectEffect[eEftType]);
		}
	}

	return 0;
}

void CInstanceBase::__ComboProcess()
{
	/*
	DWORD dwcurComboIndex = m_GraphicThingInstance.GetComboIndex();

	if (0 != dwcurComboIndex)
	{
		if (m_dwLastComboIndex != m_GraphicThingInstance.GetComboIndex())
		{
			if (!m_GraphicThingInstance.IsHandMode() & IsAffect(AFFECT_HWAYEOM))
			{
				__AttachEffect(EFFECT_FLAME_ATTACK);
			}
		}
	}

	m_dwLastComboIndex = dwcurComboIndex;
	*/
}

bool CInstanceBase::RegisterEffect(UINT eEftType, const char* c_szEftAttachBone, const char* c_szEftName, bool isCache)
{
	if (eEftType>=EFFECT_NUM)
		return false;

	ms_astAffectEffectAttachBone[eEftType]=c_szEftAttachBone;

	DWORD& rdwCRCEft=ms_adwCRCAffectEffect[eEftType];
	if (!CEffectManager::Instance().RegisterEffect2(c_szEftName, &rdwCRCEft, isCache))
	{
		TraceError("CInstanceBase::RegisterEffect(eEftType=%d, c_szEftAttachBone=%s, c_szEftName=%s, isCache=%d) - Error", eEftType, c_szEftAttachBone, c_szEftName, isCache);
		rdwCRCEft=0;
		return false;
	}

	return true;
}

void CInstanceBase::RegisterTitleName(int iIndex, const char * c_szTitleName)
{
	g_TitleNameMap.insert(std::make_pair(iIndex, c_szTitleName));
}

D3DXCOLOR __RGBToD3DXColoru(UINT r, UINT g, UINT b)
{
	DWORD dwColor=0xff;dwColor<<=8;
	dwColor|=r;dwColor<<=8;
	dwColor|=g;dwColor<<=8;
	dwColor|=b;

	return D3DXCOLOR(dwColor);
}

bool CInstanceBase::RegisterNameColor(UINT uIndex, UINT r, UINT g, UINT b)
{
	if (uIndex>=NAMECOLOR_NUM)
		return false;

	g_akD3DXClrName[uIndex]=__RGBToD3DXColoru(r, g, b);
	return true;
}

bool CInstanceBase::RegisterTitleColor(UINT uIndex, UINT r, UINT g, UINT b)
{
	if (uIndex>=TITLE_NUM)
		return false;

	g_akD3DXClrTitle[uIndex]=__RGBToD3DXColoru(r, g, b);
	return true;	
}
