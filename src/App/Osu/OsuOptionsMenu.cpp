//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		settings
//
// $NoKeywords: $
//===============================================================================//

#include "OsuOptionsMenu.h"

#include "SoundEngine.h"
#include "Engine.h"
#include "ConVar.h"
#include "Keyboard.h"
#include "Environment.h"
#include "ResourceManager.h"
#include "OpenVRInterface.h"
#include "OpenVRController.h"
#include "Mouse.h"
#include "File.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "CBaseUILabel.h"
#include "CBaseUICheckbox.h"
#include "CBaseUITextbox.h"

#include "Osu.h"
#include "OsuVR.h"
#include "OsuHUD.h"
#include "OsuSkin.h"
#include "OsuGameRules.h"
#include "OsuKeyBindings.h"
#include "OsuCircle.h"
#include "OsuSliderRenderer.h"

#include "OsuUIButton.h"
#include "OsuUISlider.h"
#include "OsuUIBackButton.h"
#include "OsuUIContextMenu.h"

#include <iostream>
#include <fstream>

ConVar osu_options_save_on_back("osu_options_save_on_back", true);

const char *OsuOptionsMenu::OSU_CONFIG_FILE_NAME = ""; // set dynamically below in the constructor



class OsuOptionsMenuSkinPreviewElement : public CBaseUIElement
{
public:
	OsuOptionsMenuSkinPreviewElement(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIElement(xPos, yPos, xSize, ySize, name) {m_osu = osu; m_iMode = 0;}

	virtual void draw(Graphics *g)
	{
		OsuSkin *skin = m_osu->getSkin();

		float hitcircleDiameter = m_vSize.y*0.5f;
		float numberScale = (hitcircleDiameter / (160.0f * (skin->isDefault12x() ? 2.0f : 1.0f))) * 1 * convar->getConVarByName("osu_number_scale_multiplier")->getFloat();
		float overlapScale = (hitcircleDiameter / (160.0f)) * 1 * convar->getConVarByName("osu_number_scale_multiplier")->getFloat();
		float scoreScale = 0.5f;

		if (m_iMode == 0)
		{
			float approachScale = clamp<float>(1.0f + 1.5f - fmod(engine->getTime()*3, 3.0f), 0.0f, 2.5f);
			float approachAlpha = clamp<float>(fmod(engine->getTime()*3, 3.0f)/1.5f, 0.0f, 1.0f);
			approachAlpha = -approachAlpha*(approachAlpha-2.0f);
			approachAlpha = -approachAlpha*(approachAlpha-2.0f);
			approachAlpha = 1.0f;

			OsuCircle::drawCircle(g, m_osu->getSkin(), m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(1.0f/5.0f), 0.0f), hitcircleDiameter, numberScale, overlapScale, 1, 42, approachScale, approachAlpha, approachAlpha, true, false);
			OsuCircle::drawHitResult(g, m_osu->getSkin(), hitcircleDiameter, hitcircleDiameter, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(2.0f/5.0f), 0.0f), OsuScore::HIT::HIT_100, 1.0f, 1.0f);
			OsuCircle::drawHitResult(g, m_osu->getSkin(), hitcircleDiameter, hitcircleDiameter, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(3.0f/5.0f), 0.0f), OsuScore::HIT::HIT_50, 1.0f, 1.0f);
			OsuCircle::drawHitResult(g, m_osu->getSkin(), hitcircleDiameter, hitcircleDiameter, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(4.0f/5.0f), 0.0f), OsuScore::HIT::HIT_MISS, 1.0f, 1.0f);
			OsuCircle::drawApproachCircle(g, m_osu->getSkin(), m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(1.0f/5.0f), 0.0f), m_osu->getSkin()->getComboColorForCounter(42), hitcircleDiameter, approachScale, approachAlpha, false, false);
		}
		else if (m_iMode == 1)
		{
			const int numNumbers = 6;
			for (int i=1; i<numNumbers+1; i++)
			{
				OsuCircle::drawHitCircleNumber(g, skin, numberScale, overlapScale, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*((float)i/(numNumbers+1.0f)), 0.0f), i-1, 1.0f);
			}
		}
		else if (m_iMode == 2)
		{
			const int numNumbers = 6;
			for (int i=1; i<numNumbers+1; i++)
			{
				Vector2 pos = m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*((float)i/(numNumbers+1.0f)), 0.0f);

				g->pushTransform();
				g->scale(scoreScale, scoreScale);
				g->translate(pos.x - skin->getScore0()->getWidth()*scoreScale, pos.y);
				m_osu->getHUD()->drawScoreNumber(g, i-1, 1.0f);
				g->popTransform();
			}
		}
	}

	virtual void onMouseUpInside()
	{
		m_iMode++;
		m_iMode = m_iMode % 3;
	}

private:
	Osu *m_osu;
	int m_iMode;
};

class OsuOptionsMenuSliderPreviewElement : public CBaseUIElement
{
public:
	OsuOptionsMenuSliderPreviewElement(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIElement(xPos, yPos, xSize, ySize, name) {m_osu = osu;}

	virtual void draw(Graphics *g)
	{
		const float hitcircleDiameter = m_vSize.y*0.5f;
		const float numberScale = (hitcircleDiameter / (160.0f * (m_osu->getSkin()->isDefault12x() ? 2.0f : 1.0f))) * 1 * convar->getConVarByName("osu_number_scale_multiplier")->getFloat();
		const float overlapScale = (hitcircleDiameter / (160.0f)) * 1 * convar->getConVarByName("osu_number_scale_multiplier")->getFloat();

		float approachScale = clamp<float>(1.0f + 1.5f - fmod(engine->getTime()*3, 3.0f), 0.0f, 2.5f);
		float approachAlpha = clamp<float>(fmod(engine->getTime()*3, 3.0f)/1.5f, 0.0f, 1.0f);
		approachAlpha = -approachAlpha*(approachAlpha-2.0f);
		approachAlpha = -approachAlpha*(approachAlpha-2.0f);
		approachAlpha = 1.0f;

		const float length = (m_vSize.x-hitcircleDiameter);
		const int numPoints = length;
		const float pointDist = length / numPoints;
		std::vector<Vector2> points;
		for (int i=0; i<numPoints; i++)
		{
			int heightAdd = i;
			if (i > numPoints/2)
				heightAdd = numPoints-i;
			float heightAddPercent = (float)heightAdd / (float)(numPoints/2.0f);
			float temp = 1.0f - heightAddPercent;
			temp *= temp;
			heightAddPercent = 1.0f - temp;

			points.push_back(Vector2(m_vPos.x + hitcircleDiameter/2 + i*pointDist, m_vPos.y + m_vSize.y/2 - hitcircleDiameter/3 + heightAddPercent*(m_vSize.y/2 - hitcircleDiameter/2)));
		}
		OsuCircle::drawCircle(g, m_osu->getSkin(), points[numPoints/2], hitcircleDiameter, numberScale, overlapScale, 2, 420, approachScale, approachAlpha, approachAlpha, true, false);
		OsuCircle::drawApproachCircle(g, m_osu->getSkin(), points[numPoints/2], m_osu->getSkin()->getComboColorForCounter(420), hitcircleDiameter, approachScale, approachAlpha, false, false);
		OsuSliderRenderer::draw(g, m_osu, points, hitcircleDiameter, 0, 1, m_osu->getSkin()->getComboColorForCounter(420));
		OsuCircle::drawSliderStartCircle(g, m_osu->getSkin(), points[0], hitcircleDiameter, numberScale, overlapScale, 1, 420);
		OsuCircle::drawSliderEndCircle(g, m_osu->getSkin(), points[points.size()-1], hitcircleDiameter, numberScale, overlapScale, 0, 0, 1.0f, 1.0f, 0.0f, false, false);
	}

private:
	Osu *m_osu;
};



OsuOptionsMenu::OsuOptionsMenu(Osu *osu) : OsuScreenBackable(osu)
{
	m_osu = osu;

	// convar callbacks
	convar->getConVarByName("osu_skin_use_skin_hitsounds")->setCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onUseSkinsSoundSamplesChange) );

	if (m_osu->isInVRMode())
		OSU_CONFIG_FILE_NAME = "osuvr";
	else
		OSU_CONFIG_FILE_NAME = "osu";

	m_osu->getNotificationOverlay()->addKeyListener(this);

	m_waitingKey = NULL;
	m_vrRenderTargetResolutionLabel = NULL;
	m_vrApproachDistanceSlider = NULL;
	m_vrVibrationStrengthSlider = NULL;
	m_vrSliderVibrationStrengthSlider = NULL;
	m_vrHudDistanceSlider = NULL;
	m_vrHudScaleSlider = NULL;

	m_fOsuFolderTextboxInvalidAnim = 0.0f;
	m_fVibrationStrengthExampleTimer = 0.0f;

	m_container = new CBaseUIContainer(0, 0, 0, 0, "");

	m_options = new CBaseUIScrollView(0, -1, 0, 0, "");
	m_options->setDrawFrame(true);
	m_options->setHorizontalScrolling(false);
	m_container->addBaseUIElement(m_options);

	m_contextMenu = new OsuUIContextMenu(m_osu, 50, 50, 150, 0, "", m_options);

	//**************************************************************************************************************************//

	addSection("General");

	addSubSection("");
	OsuUIButton *downloadOsuButton = addButton("Download osu! and get some beatmaps first!");
	downloadOsuButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onDownloadOsuClicked) );
	downloadOsuButton->setColor(0xff00ff00);
	downloadOsuButton->setTextColor(0xffffffff);

	addSubSection("osu!folder (Skins & Songs & Database)");
	m_osuFolderTextbox = addTextbox(convar->getConVarByName("osu_folder")->getString(), convar->getConVarByName("osu_folder"));
	addSpacer();
	addCheckbox("Use osu!.db database (read-only)", convar->getConVarByName("osu_database_enabled"));

	addSubSection("Player Name");
	m_nameTextbox = addTextbox(convar->getConVarByName("name")->getString(), convar->getConVarByName("name"));

	addSubSection("Window");
	addCheckbox("Pause on Focus Loss", convar->getConVarByName("osu_pause_on_focus_loss"));

	//**************************************************************************************************************************//

	addSection("Graphics");

	addSubSection("Renderer");
	addCheckbox("VSync", convar->getConVarByName("vsync"));
	addCheckbox("Show FPS Counter", convar->getConVarByName("osu_draw_fps"));
	addSpacer();
	if (!m_osu->isInVRMode())
		addCheckbox("Unlimited FPS", convar->getConVarByName("fps_unlimited"));
	CBaseUISlider *fpsSlider = addSlider("FPS Limiter:", 60.0f, 1000.0f, convar->getConVarByName("fps_max"));
	fpsSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeInt) );
	fpsSlider->setKeyDelta(60);

	addSubSection("Layout");
	OPTIONS_ELEMENT resolutionSelect = addButton("Select Resolution", UString::format("%ix%i", m_osu->getScreenWidth(), m_osu->getScreenHeight()));
	((CBaseUIButton*)resolutionSelect.elements[0])->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onResolutionSelect) );
	m_resolutionLabel = (CBaseUILabel*)resolutionSelect.elements[1];
	m_resolutionSelectButton = resolutionSelect.elements[0];
	m_fullscreenCheckbox = addCheckbox("Fullscreen");
	m_fullscreenCheckbox->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onFullscreenChange) );
	addCheckbox("Letterboxing", convar->getConVarByName("osu_letterboxing"));

	addSubSection("Detail Settings");
	addCheckbox("Snaking Sliders", convar->getConVarByName("osu_snaking_sliders"));

	//**************************************************************************************************************************//

	if (m_osu->isInVRMode())
	{
		addSection("Virtual Reality");

		addSubSection("Mixed Settings");
		addSpacer();
		m_vrRenderTargetResolutionLabel = addLabel("Final RenderTarget Resolution: %ix%i");
		addSpacer();
		CBaseUISlider *ssSlider = addSlider("SuperSampling Multiplier", 0.5f, 3.0f, convar->getConVarByName("vr_ss"), 230.0f);
		ssSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeVRSuperSampling) );
		ssSlider->setKeyDelta(0.1f);
		ssSlider->setAnimated(false);
		CBaseUISlider *aaSlider = addSlider("AntiAliasing (MSAA)", 0.0f, 16.0f, convar->getConVarByName("vr_aa"), 230.0f);
		aaSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeVRAntiAliasing) );
		aaSlider->setKeyDelta(2.0f);
		aaSlider->setAnimated(false);
		addSpacer();
		addSlider("RenderModel Brightness", 0.1f, 10.0f, convar->getConVarByName("vr_controller_model_brightness_multiplier"))->setKeyDelta(0.1f);
		addSlider("Background Brightness", 0.0f, 1.0f, convar->getConVarByName("vr_background_brightness"))->setKeyDelta(0.05f);
		addSlider("VR Cursor Opacity", 0.0f, 1.0f, convar->getConVarByName("osu_vr_cursor_alpha"))->setKeyDelta(0.1f);
		addSpacer();
		m_vrApproachDistanceSlider = addSlider("Approach Distance", 1.0f, 50.0f, convar->getConVarByName("osu_vr_approach_distance"), 175.0f);
		m_vrApproachDistanceSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeOneDecimalPlaceMeters) );
		m_vrApproachDistanceSlider->setKeyDelta(1.0f);
		m_vrHudDistanceSlider = addSlider("HUD Distance", 1.0f, 50.0f, convar->getConVarByName("osu_vr_hud_distance"), 175.0f);
		m_vrHudDistanceSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeOneDecimalPlaceMeters) );
		m_vrHudDistanceSlider->setKeyDelta(1.0f);
		m_vrHudScaleSlider = addSlider("HUD Scale", 0.1f, 10.0f, convar->getConVarByName("osu_vr_hud_scale"));
		m_vrHudScaleSlider->setKeyDelta(0.1f);
		addSpacer();
		m_vrVibrationStrengthSlider = addSlider("Vibration Strength", 0.0f, 1.0f, convar->getConVarByName("osu_vr_controller_vibration_strength"));
		m_vrVibrationStrengthSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
		m_vrVibrationStrengthSlider->setKeyDelta(0.01f);
		// TODO: enable this after SteamVR is fixed
		/*
		m_vrSliderVibrationStrengthSlider = addSlider("Slider Vibration Strength", 0.0f, 1.0f, convar->getConVarByName("osu_vr_slider_controller_vibration_strength"));
		m_vrSliderVibrationStrengthSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
		m_vrSliderVibrationStrengthSlider->setKeyDelta(0.01f);
		*/
		addSpacer();
		addCheckbox("Spectator Camera (ALT + C)", convar->getConVarByName("vr_spectator_mode"));
		addCheckbox("Auto Switch Primary Controller", convar->getConVarByName("vr_auto_switch_primary_controller"));
		addSpacer();
		addCheckbox("Draw HMD to Window", convar->getConVarByName("vr_draw_hmd_to_window"));
		addCheckbox("Draw Both Eyes to Window", convar->getConVarByName("vr_draw_hmd_to_window_draw_both_eyes"));
		addCheckbox("Draw Floor", convar->getConVarByName("osu_vr_draw_floor"));
		addCheckbox("Draw Controller Models", convar->getConVarByName("vr_draw_controller_models"));
		addCheckbox("Draw Lighthouse Models", convar->getConVarByName("vr_draw_lighthouse_models"));
		addSpacer();
		addCheckbox("Draw VR Playfield", convar->getConVarByName("osu_vr_draw_playfield"));
		addCheckbox("Draw Desktop Playfield", convar->getConVarByName("osu_vr_draw_desktop_playfield"));
		addSpacer();
		addCheckbox("Controller Distance Color Warning", convar->getConVarByName("osu_vr_controller_warning_distance_enabled"));
		addCheckbox("Show Tutorial on Startup", convar->getConVarByName("osu_vr_tutorial"));
		addSpacer();
	}

	//**************************************************************************************************************************//

	addSection("Audio");

	addSubSection("Devices");
	OPTIONS_ELEMENT outputDeviceSelect = addButton("Select Output Device", "Default");
	((CBaseUIButton*)outputDeviceSelect.elements[0])->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onOutputDeviceSelect) );
	m_outputDeviceSelectButton = outputDeviceSelect.elements[0];
	m_outputDeviceLabel = (CBaseUILabel*)outputDeviceSelect.elements[1];

	addSubSection("Volume");
	addSlider("Master:", 0.0f, 1.0f, convar->getConVarByName("osu_volume_master"), 70.0f);
	addSlider("Music:", 0.0f, 1.0f, convar->getConVarByName("osu_volume_music"), 70.0f);
	addSlider("Effects:", 0.0f, 1.0f, convar->getConVarByName("osu_volume_effects"), 70.0f);

	addSubSection("Offset Adjustment");
	CBaseUISlider *offsetSlider = addSlider("Global Offset:", -300.0f, 300.0f, convar->getConVarByName("osu_global_offset"));
	offsetSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeInt) );
	offsetSlider->setKeyDelta(1);

	//**************************************************************************************************************************//

	addSection("Input");

	addSubSection("Mouse");
	addSlider("Sensitivity:", 0.4f, 6.0f, convar->getConVarByName("mouse_sensitivity"))->setKeyDelta(0.1f);
	addCheckbox("Raw Input", convar->getConVarByName("mouse_raw_input"));
	addCheckbox("Map Absolute Raw Input to Window", convar->getConVarByName("mouse_raw_input_absolute_to_window"));
	addCheckbox("Confine Cursor (Windowed)", convar->getConVarByName("osu_confine_cursor_windowed"));
	addCheckbox("Confine Cursor (Fullscreen)", convar->getConVarByName("osu_confine_cursor_fullscreen"));
	addCheckbox("Disable Mouse Wheel in Play Mode", convar->getConVarByName("osu_disable_mousewheel"));
	addCheckbox("Disable Mouse Buttons in Play Mode", convar->getConVarByName("osu_disable_mousebuttons"));

	addSubSection("Tablet");
	addCheckbox("OS TabletPC Support", convar->getConVarByName("win_realtimestylus"));
	addCheckbox("Windows Ink Workaround", convar->getConVarByName("win_ink_workaround"));
	addCheckbox("Ignore Sensitivity & Raw Input", convar->getConVarByName("tablet_sensitivity_ignore"));

	addSpacer();
	addSubSection("Keyboard");
	addSubSection("Keys - osu! Standard Mode");
	addButton("Left Click", &OsuKeyBindings::LEFT_CLICK)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Right Click", &OsuKeyBindings::RIGHT_CLICK)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addSubSection("Keys - In-Game");
	addButton("Game Pause", &OsuKeyBindings::GAME_PAUSE)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Skip Cutscene", &OsuKeyBindings::SKIP_CUTSCENE)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Scrubbing (+ Click Drag!)", &OsuKeyBindings::SEEK_TIME)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Increase Local Song Offset", &OsuKeyBindings::INCREASE_LOCAL_OFFSET)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Decrease Local Song Offset", &OsuKeyBindings::DECREASE_LOCAL_OFFSET)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Quick Retry (hold briefly)", &OsuKeyBindings::QUICK_RETRY)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Quick Save", &OsuKeyBindings::QUICK_SAVE)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Quick Load", &OsuKeyBindings::QUICK_LOAD)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addSubSection("Keys - Universal");
	addButton("Save Screenshot", &OsuKeyBindings::SAVE_SCREENSHOT)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Increase Volume", &OsuKeyBindings::INCREASE_VOLUME)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Decrease Volume", &OsuKeyBindings::DECREASE_VOLUME)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Disable Mouse Buttons", &OsuKeyBindings::DISABLE_MOUSE_BUTTONS)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Boss Key (Minimize)", &OsuKeyBindings::BOSS_KEY)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addSubSection("Keys - Mod Select");
	addButton("Easy", &OsuKeyBindings::MOD_EASY)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("No Fail", &OsuKeyBindings::MOD_NOFAIL)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Half Time", &OsuKeyBindings::MOD_HALFTIME)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Hard Rock", &OsuKeyBindings::MOD_HARDROCK)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Sudden Death", &OsuKeyBindings::MOD_SUDDENDEATH)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Double Time", &OsuKeyBindings::MOD_DOUBLETIME)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Hidden", &OsuKeyBindings::MOD_HIDDEN)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Flashlight", &OsuKeyBindings::MOD_FLASHLIGHT)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Relax", &OsuKeyBindings::MOD_RELAX)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Autopilot", &OsuKeyBindings::MOD_AUTOPILOT)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Spunout", &OsuKeyBindings::MOD_SPUNOUT)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Auto", &OsuKeyBindings::MOD_AUTO)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addSpacer();

	//**************************************************************************************************************************//

	addSection("Skin");

	addSubSection("Skin");
	addSkinPreview();
	OPTIONS_ELEMENT skinSelect = addButton("Select Skin", "default");
	((CBaseUIButton*)skinSelect.elements[0])->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSkinSelect) );
	m_skinLabel = (CBaseUILabel*)skinSelect.elements[1];
	m_skinSelectButton = skinSelect.elements[0];
	addButton("Reload Skin (CTRL+ALT+SHIFT+S)")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSkinReload) );
	addSpacer();
	addSlider("Number Scale:", 0.01f, 3.0f, convar->getConVarByName("osu_number_scale_multiplier"), 135.0f)->setKeyDelta(0.1f);
	addSlider("HitResult Scale:", 0.01f, 3.0f, convar->getConVarByName("osu_hitresult_scale"), 135.0f)->setKeyDelta(0.1f);
	addCheckbox("Draw Numbers", convar->getConVarByName("osu_draw_numbers"));
	addCheckbox("Draw ApproachCircles", convar->getConVarByName("osu_draw_approach_circles"));
	addSpacer();
	addCheckbox("Ignore Beatmap Sample Volume", convar->getConVarByName("osu_ignore_beatmap_sample_volume"));
	addCheckbox("Ignore Beatmap Combo Colors", convar->getConVarByName("osu_ignore_beatmap_combo_colors"));
	addCheckbox("Use skin's sound samples", convar->getConVarByName("osu_skin_use_skin_hitsounds"));
	addCheckbox("Load HD @2x", convar->getConVarByName("osu_skin_hd"));
	addSpacer();
	addCheckbox("Draw CursorTrail", convar->getConVarByName("osu_draw_cursor_trail"));
	m_cursorSizeSlider = addSlider("Cursor Size:", 0.01f, 5.0f, convar->getConVarByName("osu_cursor_scale"));
	m_cursorSizeSlider->setAnimated(false);
	m_cursorSizeSlider->setKeyDelta(0.1f);
	addSpacer();
	addSliderPreview();
	addCheckbox("Use slidergradient.png", convar->getConVarByName("osu_slider_use_gradient_image"));
	addCheckbox("Use osu!next Slider Style", convar->getConVarByName("osu_slider_osu_next_style"));
	addCheckbox("Use combo color as tint for slider ball", convar->getConVarByName("osu_slider_ball_tint_combo_color"));
	addCheckbox("Use combo color as tint for slider border", convar->getConVarByName("osu_slider_border_tint_combo_color"));
	addCheckbox("Draw SliderEndCircle", convar->getConVarByName("osu_slider_draw_endcircle"));
	addSlider("Slider Opacity", 0.0f, 1.0f, convar->getConVarByName("osu_slider_alpha_multiplier"));
	addSlider("SliderBody Opacity", 0.0f, 1.0f, convar->getConVarByName("osu_slider_body_alpha_multiplier"));
	addSlider("SliderBody Color Saturation", 0.0f, 1.0f, convar->getConVarByName("osu_slider_body_color_saturation"));
	addSlider("SliderBorder Size", 0.0f, 9.0f, convar->getConVarByName("osu_slider_border_size_multiplier"))->setKeyDelta(0.1f);

	//**************************************************************************************************************************//

	addSection("Gameplay");

	addSubSection("General");
	addSlider("Background Dim:", 0.0f, 1.0f, convar->getConVarByName("osu_background_dim"))->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_backgroundBrightnessSlider = addSlider("Background Brightness:", 0.0f, 1.0f, convar->getConVarByName("osu_background_brightness"));
	m_backgroundBrightnessSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	addCheckbox("FadeIn Background during Breaks", convar->getConVarByName("osu_background_fade_during_breaks"));
	addCheckbox("Show Approach Circle on First \"Hidden\" Object", convar->getConVarByName("osu_show_approach_circle_on_first_hidden_object"));
	addCheckbox("Note Blocking (Noob Protection)", convar->getConVarByName("osu_note_blocking"));
	addCheckbox("Show pp on ranking screen", convar->getConVarByName("osu_rankingscreen_pp"));

	addSubSection("HUD");
	addCheckbox("Draw HUD", convar->getConVarByName("osu_draw_hud"));
	addCheckbox("Draw Combo", convar->getConVarByName("osu_draw_combo"));
	addCheckbox("Draw Accuracy", convar->getConVarByName("osu_draw_accuracy"));
	addCheckbox("Draw ProgressBar", convar->getConVarByName("osu_draw_progressbar"));
	addCheckbox("Draw HitErrorBar", convar->getConVarByName("osu_draw_hiterrorbar"));
	addCheckbox("Draw Miss Window on HitErrorBar", convar->getConVarByName("osu_hud_hiterrorbar_showmisswindow"));
	addCheckbox("Draw Scrubbing Timeline", convar->getConVarByName("osu_draw_scrubbing_timeline"));
	addSpacer();
	addCheckbox("Draw Stats: pp (Performance Points)", convar->getConVarByName("osu_draw_statistics_pp"));
	addCheckbox("Draw Stats: Misses", convar->getConVarByName("osu_draw_statistics_misses"));
	addCheckbox("Draw Stats: BPM", convar->getConVarByName("osu_draw_statistics_bpm"));
	addCheckbox("Draw Stats: AR", convar->getConVarByName("osu_draw_statistics_ar"));
	addCheckbox("Draw Stats: CS", convar->getConVarByName("osu_draw_statistics_cs"));
	addCheckbox("Draw Stats: OD", convar->getConVarByName("osu_draw_statistics_od"));
	addCheckbox("Draw Stats: Notes Per Second", convar->getConVarByName("osu_draw_statistics_nps"));
	addCheckbox("Draw Stats: Note Density", convar->getConVarByName("osu_draw_statistics_nd"));
	addCheckbox("Draw Stats: Unstable Rate", convar->getConVarByName("osu_draw_statistics_ur"));
	addSpacer();
	m_hudSizeSlider = addSlider("HUD Scale:", 0.1f, 3.0f, convar->getConVarByName("osu_hud_scale"), 165.0f);
	m_hudSizeSlider->setKeyDelta(0.1f);
	addSpacer();
	m_hudComboScaleSlider = addSlider("Combo Scale:", 0.1f, 3.0f, convar->getConVarByName("osu_hud_combo_scale"), 165.0f);
	m_hudComboScaleSlider->setKeyDelta(0.1f);
	m_hudScoreScaleSlider = addSlider("Score Scale:", 0.1f, 3.0f, convar->getConVarByName("osu_hud_score_scale"), 165.0f);
	m_hudScoreScaleSlider->setKeyDelta(0.1f);
	m_hudAccuracyScaleSlider = addSlider("Accuracy Scale:", 0.1f, 3.0f, convar->getConVarByName("osu_hud_accuracy_scale"), 165.0f);
	m_hudAccuracyScaleSlider->setKeyDelta(0.1f);
	m_hudHiterrorbarScaleSlider = addSlider("HitErrorBar Scale:", 0.1f, 3.0f, convar->getConVarByName("osu_hud_hiterrorbar_scale"), 165.0f);
	m_hudHiterrorbarScaleSlider->setKeyDelta(0.1f);
	m_hudProgressbarScaleSlider = addSlider("ProgressBar Scale:", 0.1f, 3.0f, convar->getConVarByName("osu_hud_progressbar_scale"), 165.0f);
	m_hudProgressbarScaleSlider->setKeyDelta(0.1f);
	m_statisticsOverlayScaleSlider = addSlider("Statistics Scale:", 0.1f, 3.0f, convar->getConVarByName("osu_hud_statistics_scale"), 165.0f);
	m_statisticsOverlayScaleSlider->setKeyDelta(0.1f);

	addSubSection("Playfield");
	addCheckbox("Draw FollowPoints", convar->getConVarByName("osu_draw_followpoints"));
	addCheckbox("Draw Playfield Border", convar->getConVarByName("osu_draw_playfield_border"));
	addSpacer();
	m_playfieldBorderSizeSlider = addSlider("Playfield Border Size:", 0.0f, 500.0f, convar->getConVarByName("osu_hud_playfield_border_size"));
	m_playfieldBorderSizeSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeInt) );
	m_playfieldBorderSizeSlider->setKeyDelta(1.0f);

	addSubSection("Hitobjects");
	addCheckbox("Shrinking Sliders", convar->getConVarByName("osu_slider_shrink"));
	addCheckbox("Use New Hidden Fading Sliders", convar->getConVarByName("osu_mod_hd_slider_fade"));
	addCheckbox("Use Fast Hidden Fading Sliders (!)", convar->getConVarByName("osu_mod_hd_slider_fast_fade"));
	addCheckbox("Use Score V2 slider accuracy", convar->getConVarByName("osu_slider_scorev2"));

	//**************************************************************************************************************************//

	addSection("Bullshit");

	addSubSection("Why");
	addCheckbox("Rainbow ApproachCircles", convar->getConVarByName("osu_circle_rainbow"));
	addCheckbox("Rainbow Sliders", convar->getConVarByName("osu_slider_rainbow"));
	addCheckbox("Rainbow Numbers", convar->getConVarByName("osu_circle_number_rainbow"));
	addCheckbox("SliderBreak Epilepsy", convar->getConVarByName("osu_slider_break_epilepsy"));
	addCheckbox("Invisible Cursor", convar->getConVarByName("osu_hide_cursor_during_gameplay"));
	addCheckbox("Draw 300s", convar->getConVarByName("osu_hitresult_draw_300s"));



	// the context menu gets added last (drawn on top of everything)
	m_options->getContainer()->addBaseUIElement(m_contextMenu);
}

OsuOptionsMenu::~OsuOptionsMenu()
{
	SAFE_DELETE(m_container);
}

void OsuOptionsMenu::draw(Graphics *g)
{
	if (!m_bVisible) return;

	// interactive sliders

	if (m_backgroundBrightnessSlider->isActive())
	{
		short brightness = clamp<float>(m_backgroundBrightnessSlider->getFloat(), 0.0f, 1.0f)*255.0f;
		g->setColor(COLOR(255, brightness, brightness, brightness));
		g->fillRect(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight());
	}

	m_container->draw(g);

	if (m_hudSizeSlider->isActive() || m_hudComboScaleSlider->isActive() || m_hudScoreScaleSlider->isActive() || m_hudAccuracyScaleSlider->isActive() || m_hudHiterrorbarScaleSlider->isActive() || m_hudProgressbarScaleSlider->isActive() || m_statisticsOverlayScaleSlider->isActive())
		m_osu->getHUD()->drawDummy(g);
	else if (m_playfieldBorderSizeSlider->isActive())
		m_osu->getHUD()->drawPlayfieldBorder(g, OsuGameRules::getPlayfieldCenter(m_osu), OsuGameRules::getPlayfieldSize(m_osu), 100);
	else
		OsuScreenBackable::draw(g);

	if (m_cursorSizeSlider->getFloat() < 0.15f)
		engine->getMouse()->drawDebug(g);
}

void OsuOptionsMenu::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	m_container->update();

	// force context menu focus
	if (m_contextMenu->isVisible())
	{
		std::vector<CBaseUIElement*> *options = m_options->getContainer()->getAllBaseUIElementsPointer();
		for (int i=0; i<options->size(); i++)
		{
			CBaseUIElement *e = ((*options)[i]);
			if (e == m_contextMenu)
				continue;

			e->stealFocus();
		}
	}

	// flash osu!folder textbox red if incorrect
	if (m_fOsuFolderTextboxInvalidAnim > engine->getTime())
	{
		char redness = std::abs(std::sin((m_fOsuFolderTextboxInvalidAnim - engine->getTime())*3))*128;
		m_osuFolderTextbox->setBackgroundColor(COLOR(255, redness, 0, 0));
	}
	else
		m_osuFolderTextbox->setBackgroundColor(0xff000000);

	// demo vibration strength while sliding
	if (m_vrVibrationStrengthSlider != NULL && m_vrVibrationStrengthSlider->isActive())
	{
		if (engine->getTime() > m_fVibrationStrengthExampleTimer)
		{
			m_fVibrationStrengthExampleTimer = engine->getTime() + 0.65f;

			openvr->getController()->triggerHapticPulse(m_osu->getVR()->getHapticPulseStrength());
		}
	}
	if (m_vrSliderVibrationStrengthSlider != NULL && m_vrSliderVibrationStrengthSlider->isActive())
	{
		openvr->getController()->triggerHapticPulse(m_osu->getVR()->getSliderHapticPulseStrength());
	}
}

void OsuOptionsMenu::onKeyDown(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	m_container->onKeyDown(e);
	if (e.isConsumed()) return;

	if (e == KEY_ESCAPE || e == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt())
	{
		if (m_contextMenu->isVisible())
			m_contextMenu->setVisible2(false);
		else
			onBack();
	}

	e.consume();
}

void OsuOptionsMenu::onKeyUp(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	m_container->onKeyUp(e);
	if (e.isConsumed()) return;
}

void OsuOptionsMenu::onChar(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	m_container->onChar(e);
	if (e.isConsumed()) return;
}

void OsuOptionsMenu::onResolutionChange(Vector2 newResolution)
{
	OsuScreenBackable::onResolutionChange(newResolution);

	m_resolutionLabel->setText(UString::format("%ix%i", (int)newResolution.x, (int)newResolution.y));
}

void OsuOptionsMenu::onKey(KeyboardEvent &e)
{
	// from OsuNotificationOverlay
	if (m_waitingKey != NULL)
	{
		m_waitingKey->setValue((float)e.getKeyCode());
		m_waitingKey = NULL;
	}
}

void OsuOptionsMenu::setVisible(bool visible)
{
	m_bVisible = visible;

	if (visible)
		updateLayout();
	else
		m_contextMenu->setVisible2(false);
}

bool OsuOptionsMenu::shouldDrawVRDummyHUD()
{
	return isVisible();
	/*return m_vrHudDistanceSlider != NULL && m_vrHudScaleSlider != NULL && (m_vrHudDistanceSlider->isActive() || m_vrHudScaleSlider->isActive());*/
}

void OsuOptionsMenu::updateLayout()
{
	// set all elements to the current convar values
	for (int i=0; i<m_elements.size(); i++)
	{
		switch (m_elements[i].type)
		{
		case 5: // checkbox
			if (m_elements[i].cvar != NULL)
			{
				for (int e=0; e<m_elements[i].elements.size(); e++)
				{
					CBaseUICheckbox *checkboxPointer = dynamic_cast<CBaseUICheckbox*>(m_elements[i].elements[e]);
					if (checkboxPointer != NULL)
						checkboxPointer->setChecked(m_elements[i].cvar->getBool());
				}
			}
			break;
		case 6: // slider
			if (m_elements[i].cvar != NULL)
			{
				if (m_elements[i].elements.size() == 3)
				{
					CBaseUISlider *sliderPointer = dynamic_cast<CBaseUISlider*>(m_elements[i].elements[1]);
					if (sliderPointer != NULL)
					{
						sliderPointer->setValue(m_elements[i].cvar->getFloat(), false);
						sliderPointer->forceCallCallback();
					}
				}
			}
			break;
		case 7: // textbox
			if (m_elements[i].cvar != NULL)
			{
				if (m_elements[i].elements.size() == 1)
				{
					CBaseUITextbox *textboxPointer = dynamic_cast<CBaseUITextbox*>(m_elements[i].elements[0]);
					if (textboxPointer != NULL)
						textboxPointer->setText(m_elements[i].cvar->getString());
				}
			}
			break;
		}
	}

	m_fullscreenCheckbox->setChecked(env->isFullscreen(), false);

	updateVRRenderTargetResolutionLabel();

	// HACKHACK: temp, not correct anymore if loading fails
	m_skinLabel->setText(convar->getConVarByName("osu_skin")->getString());
	m_outputDeviceLabel->setText(engine->getSound()->getOutputDevice());

	//************************************************************************************************************************************//

	OsuScreenBackable::updateLayout();

	m_container->setSize(m_osu->getScreenSize());

	// options panel
	int optionsWidth = (int)(m_osu->getScreenWidth()*0.525f);
	m_options->setRelPosX(m_osu->getScreenWidth()/2 - optionsWidth/2);
	m_options->setSize(optionsWidth, m_osu->getScreenHeight()+1);

	bool enableHorizontalScrolling = false;
	int sideMargin = 25*2;
	int spaceSpacing = 25;
	int sectionSpacing = -15; // section title to first element
	int subsectionSpacing = 15; // subsection title to first element
	int sectionEndSpacing = 70; // last section element to next section title
	int subsectionEndSpacing = 65; // last subsection element to next subsection title
	int elementSpacing = 5;
	int elementTextStartOffset = 11; // e.g. labels in front of sliders
	int yCounter = sideMargin;
	for (int i=0; i<m_elements.size(); i++)
	{
		int elementWidth = optionsWidth - 2*sideMargin - 2;

		if (m_elements[i].elements.size() == 1)
		{
			CBaseUIElement *e = m_elements[i].elements[0];

			CBaseUICheckbox *checkboxPointer = dynamic_cast<CBaseUICheckbox*>(e);
			if (checkboxPointer != NULL)
			{
				checkboxPointer->setWidthToContent(0);
				if (checkboxPointer->getSize().x > elementWidth)
					enableHorizontalScrolling = true;
				else
					e->setSizeX(elementWidth);
			}
			else
				e->setSizeX(elementWidth);

			int sideMarginAdd = 0;
			CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(e);
			if (labelPointer != NULL)
				sideMarginAdd += elementTextStartOffset;

			e->setRelPosX(sideMargin + sideMarginAdd);
			e->setRelPosY(yCounter);

			yCounter += e->getSize().y;
		}
		else if (m_elements[i].elements.size() == 2)
		{
			CBaseUIElement *e1 = m_elements[i].elements[0];
			CBaseUIElement *e2 = m_elements[i].elements[1];

			int spacing = 15;

			e1->setRelPos(sideMargin, yCounter);
			e1->setSizeX(elementWidth/2 - spacing);

			e2->setRelPos(sideMargin + e1->getSize().x + 2*spacing, yCounter);
			e2->setSizeX(elementWidth/2 - spacing);

			yCounter += e1->getSize().y;
		}
		else if (m_elements[i].elements.size() == 3)
		{
			CBaseUIElement *e1 = m_elements[i].elements[0];
			CBaseUIElement *e2 = m_elements[i].elements[1];
			CBaseUIElement *e3 = m_elements[i].elements[2];

			int labelSliderLabelOffset = 15;
			int sliderSize = elementWidth - e1->getSize().x - e3->getSize().x;

			if (sliderSize < 100)
			{
				enableHorizontalScrolling = true;
				sliderSize = 100;
			}

			e1->setRelPos(sideMargin + elementTextStartOffset, yCounter);

			e2->setRelPos(sideMargin + elementTextStartOffset + e1->getSize().x + labelSliderLabelOffset, yCounter);
			e2->setSizeX(sliderSize - 2*elementTextStartOffset - labelSliderLabelOffset*2);

			e3->setRelPos(sideMargin + elementTextStartOffset + e1->getSize().x + e2->getSize().x + 2*labelSliderLabelOffset, yCounter);
			///e3->setSizeX(valueLabelSize - valueLabelOffset);

			yCounter += e2->getSize().y;
		}

		switch (m_elements[i].type)
		{
		case 0:
			yCounter += spaceSpacing;
			break;
		case 1:
			yCounter += sectionSpacing;
			break;
		case 2:
			yCounter += subsectionSpacing;
			break;
		default:
			break;
		}

		yCounter += elementSpacing;

		// if the next element is a new section, add even more spacing
		if (i+1 < m_elements.size() && m_elements[i+1].type == 1)
			yCounter += sectionEndSpacing;

		// if the next element is a new subsection, add even more spacing
		if (i+1 < m_elements.size() && m_elements[i+1].type == 2)
			yCounter += subsectionEndSpacing;
	}
	m_options->setScrollSizeToContent();
	m_options->setHorizontalScrolling(enableHorizontalScrolling);

	m_container->update_pos();
}

void OsuOptionsMenu::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	m_osu->getNotificationOverlay()->stopWaitingForKey();
	m_osu->toggleOptionsMenu();

	save();
}

void OsuOptionsMenu::updateOsuFolder()
{
	// automatically insert a slash at the end if the user forgets
	// i don't know how i feel about this code
	UString newOsuFolder = m_osuFolderTextbox->getText();
	newOsuFolder = newOsuFolder.trim();
	if (newOsuFolder.length() > 0)
	{
		const wchar_t *uFolder = newOsuFolder.wc_str();
		if (uFolder[newOsuFolder.length()-1] != L'/' && uFolder[newOsuFolder.length()-1] != L'\\')
		{
			newOsuFolder.append("/");
			m_osuFolderTextbox->setText(newOsuFolder);
		}
	}

	// set convar to new folder
	convar->getConVarByName("osu_folder")->setValue(newOsuFolder);
}

void OsuOptionsMenu::updateName()
{
	convar->getConVarByName("name")->setValue(m_nameTextbox->getText());
}

void OsuOptionsMenu::updateVRRenderTargetResolutionLabel()
{
	if (m_vrRenderTargetResolutionLabel == NULL || !openvr->isReady() || !m_osu->isInVRMode())
		return;

	Vector2 vrRenderTargetResolution = openvr->getRenderTargetResolution();
	m_vrRenderTargetResolutionLabel->setText(UString::format(m_vrRenderTargetResolutionLabel->getName().toUtf8(), (int)vrRenderTargetResolution.x, (int)vrRenderTargetResolution.y));
}

void OsuOptionsMenu::onFullscreenChange(CBaseUICheckbox *checkbox)
{
	if (checkbox->isChecked())
		env->enableFullscreen();
	else
		env->disableFullscreen();
}

void OsuOptionsMenu::onSkinSelect()
{
	updateOsuFolder();

	UString skinFolder = convar->getConVarByName("osu_folder")->getString();
	skinFolder.append("Skins/");
	std::vector<UString> skinFolders = env->getFoldersInFolder(skinFolder);

	if (skinFolders.size() > 0)
	{
		m_contextMenu->setPos(m_skinSelectButton->getPos());
		m_contextMenu->setRelPos(m_skinSelectButton->getRelPos());
		m_contextMenu->begin();
		m_contextMenu->addButton("default");
		m_contextMenu->addButton("defaultvr");
		for (int i=0; i<skinFolders.size(); i++)
		{
			if (skinFolders[i] == "." || skinFolders[i] == "..") // is this universal in every file system? too lazy to check. should probably fix this in the engine and not here
				continue;

			m_contextMenu->addButton(skinFolders[i]);
		}
		m_contextMenu->end();
		m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSkinSelect2) );
	}
	else
	{
		m_osu->getNotificationOverlay()->addNotification("Error: Couldn't find any skins", 0xffff0000);
		m_options->scrollToTop();
		m_fOsuFolderTextboxInvalidAnim = engine->getTime() + 3.0f;
	}
}

void OsuOptionsMenu::onSkinSelect2(UString skinName)
{
	convar->getConVarByName("osu_skin")->setValue(skinName);
	m_skinLabel->setText(skinName);
}

void OsuOptionsMenu::onSkinReload()
{
	m_osu->reloadSkin();
}

void OsuOptionsMenu::onResolutionSelect()
{
	std::vector<Vector2> resolutions;

	// 4:3
	resolutions.push_back(Vector2(800, 600));
    resolutions.push_back(Vector2(1024, 768));
    resolutions.push_back(Vector2(1152, 864));
    resolutions.push_back(Vector2(1280, 960));
    resolutions.push_back(Vector2(1280, 1024));
    resolutions.push_back(Vector2(1600, 1200));
    resolutions.push_back(Vector2(1920, 1440));
    resolutions.push_back(Vector2(2560, 1920));

    // 16:9 and 16:10
    resolutions.push_back(Vector2(1024, 600));
    resolutions.push_back(Vector2(1280, 720));
    resolutions.push_back(Vector2(1280, 768));
    resolutions.push_back(Vector2(1280, 800));
    resolutions.push_back(Vector2(1360, 768));
    resolutions.push_back(Vector2(1366, 768));
    resolutions.push_back(Vector2(1440, 900));
    resolutions.push_back(Vector2(1600, 900));
    resolutions.push_back(Vector2(1600, 1024));
    resolutions.push_back(Vector2(1680, 1050));
    resolutions.push_back(Vector2(1920, 1080));
    resolutions.push_back(Vector2(1920, 1200));
    resolutions.push_back(Vector2(2560, 1440));
    resolutions.push_back(Vector2(2560, 1600));
    resolutions.push_back(Vector2(3840, 2160));
    resolutions.push_back(Vector2(5120, 2880));
    resolutions.push_back(Vector2(7680, 4320));

    // wtf
    resolutions.push_back(Vector2(4096, 2160));

    // get custom resolutions
    std::vector<Vector2> customResolutions;
    std::ifstream customres("cfg/customres.cfg");
	std::string curLine;
	while (std::getline(customres, curLine))
	{
		const char *curLineChar = curLine.c_str();
		if (curLine.find("//") == std::string::npos) // ignore comments
		{
			int width = 0;
			int height = 0;
			if (sscanf(curLineChar, "%ix%i\n", &width, &height) == 2)
			{
				if (width > 319 && height > 239) // 320x240 sanity check
					customResolutions.push_back(Vector2(width, height));
			}
		}
	}

    // native resolution at the end
    Vector2 nativeResolution = env->getNativeScreenSize();
    bool containsNativeResolution = false;
    for (int i=0; i<resolutions.size(); i++)
    {
    	if (resolutions[i] == nativeResolution)
    	{
    		containsNativeResolution = true;
    		break;
    	}
    }
    if (!containsNativeResolution)
    	resolutions.push_back(nativeResolution);

    // build context menu
	m_contextMenu->setPos(m_resolutionSelectButton->getPos());
	m_contextMenu->setRelPos(m_resolutionSelectButton->getRelPos());
	m_contextMenu->begin();
	for (int i=0; i<resolutions.size(); i++)
	{
		if (resolutions[i].x > nativeResolution.x || resolutions[i].y > nativeResolution.y)
			continue;

		m_contextMenu->addButton(UString::format("%ix%i", (int)std::round(resolutions[i].x), (int)std::round(resolutions[i].y)));
	}
	for (int i=0; i<customResolutions.size(); i++)
	{
		m_contextMenu->addButton(UString::format("%ix%i", (int)std::round(customResolutions[i].x), (int)std::round(customResolutions[i].y)));
	}
	m_contextMenu->end();
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onResolutionSelect2) );
}

void OsuOptionsMenu::onResolutionSelect2(UString resolution)
{
	if (env->isFullscreen())
		convar->getConVarByName("osu_resolution")->setValue(resolution);
	else
		convar->getConVarByName("windowed")->execArgs(resolution);
}

void OsuOptionsMenu::onOutputDeviceSelect()
{
	std::vector<UString> outputDevices = engine->getSound()->getOutputDevices();

    // build context menu
	m_contextMenu->setPos(m_outputDeviceSelectButton->getPos());
	m_contextMenu->setRelPos(m_outputDeviceSelectButton->getRelPos());
	m_contextMenu->begin();
	for (int i=0; i<outputDevices.size(); i++)
	{
		m_contextMenu->addButton(outputDevices[i]);
	}
	m_contextMenu->end();
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onOutputDeviceSelect2) );
}

void OsuOptionsMenu::onOutputDeviceSelect2(UString outputDeviceName)
{
	engine->getSound()->setOutputDevice(outputDeviceName);
	m_outputDeviceLabel->setText(engine->getSound()->getOutputDevice());
	m_osu->reloadSkin(); // needed to reload sounds
}

void OsuOptionsMenu::onDownloadOsuClicked()
{
	env->openURLInDefaultBrowser("https://osu.ppy.sh/");
}

void OsuOptionsMenu::onCheckboxChange(CBaseUICheckbox *checkbox)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == checkbox)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(checkbox->isChecked());
				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChange(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*100.0f)/100.0f); // round to 2 decimal places

				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					labelPointer->setText(m_elements[i].cvar->getString());
				}
				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeOneDecimalPlace(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*10.0f)/10.0f); // round to 1 decimal place

				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					labelPointer->setText(m_elements[i].cvar->getString());
				}
				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeOneDecimalPlaceMeters(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*10.0f)/10.0f); // round to 1 decimal place

				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					labelPointer->setText(UString::format("%.1f m", m_elements[i].cvar->getFloat()));
				}
				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeInt(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat())); // round to int

				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					labelPointer->setText(m_elements[i].cvar->getString());
				}
				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangePercent(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*100.0f)/100.0f);

				if (m_elements[i].elements.size() == 3)
				{
					int percent = std::round(m_elements[i].cvar->getFloat()*100.0f);

					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					labelPointer->setText(UString::format("%i%%", percent));
				}
				break;
			}
		}
	}
}

void OsuOptionsMenu::onKeyBindingButtonPressed(CBaseUIButton *button)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == button)
			{
				if (m_elements[i].cvar != NULL)
				{
					m_waitingKey = m_elements[i].cvar;

					UString notificationText = "Press new key for ";
					notificationText.append(button->getText());
					notificationText.append(":");
					m_osu->getNotificationOverlay()->addNotification(notificationText, 0xffffffff, true);
				}
				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeVRSuperSampling(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*10.0f)/10.0f); // round to 1 decimal place

				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					UString labelText = m_elements[i].cvar->getString();
					labelText.append("x");
					labelPointer->setText(labelText);
				}
				break;
			}
		}
	}

	updateVRRenderTargetResolutionLabel();
}

void OsuOptionsMenu::onSliderChangeVRAntiAliasing(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				int number = std::round(slider->getFloat()); // round to int
				int aa = 0;
				if (number > 8)
					aa = 16;
				else if (number > 4)
					aa = 8;
				else if (number > 2)
					aa = 4;
				else if (number > 0)
					aa = 2;

				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(aa);

				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					labelPointer->setText(UString::format("%ix", aa));
				}
				break;
			}
		}
	}
}

void OsuOptionsMenu::onUseSkinsSoundSamplesChange(UString oldValue, UString newValue)
{
	m_osu->reloadSkin();
}

void OsuOptionsMenu::addSpacer()
{
	OPTIONS_ELEMENT e;
	e.type = 0;
	e.cvar = NULL;
	m_elements.push_back(e);
}

CBaseUILabel *OsuOptionsMenu::addSection(UString text)
{
	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 25, text, text);
	label->setFont(m_osu->getTitleFont());
	label->setSizeToContent(0, 0);
	label->setTextJustification(CBaseUILabel::TEXT_JUSTIFICATION_RIGHT);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(label);
	e.type = 1;
	e.cvar = NULL;
	m_elements.push_back(e);

	return label;
}

CBaseUILabel *OsuOptionsMenu::addSubSection(UString text)
{
	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 25, text, text);
	label->setFont(m_osu->getSubTitleFont());
	label->setSizeToContent(0, 0);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(label);
	e.type = 2;
	e.cvar = NULL;
	m_elements.push_back(e);

	return label;
}

CBaseUILabel *OsuOptionsMenu::addLabel(UString text)
{
	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 25, text, text);
	label->setSizeToContent(0, 0);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(label);
	e.type = 3;
	e.cvar = NULL;
	m_elements.push_back(e);

	return label;
}

OsuUIButton *OsuOptionsMenu::addButton(UString text, ConVar *cvar)
{
	OsuUIButton *button = new OsuUIButton(m_osu, 0, 0, m_options->getSize().x, 50, text, text);
	button->setColor(0xff0e94b5);
	button->setUseDefaultSkin();
	m_options->getContainer()->addBaseUIElement(button);

	OPTIONS_ELEMENT e;
	e.elements.push_back(button);
	e.type = 4;
	e.cvar = cvar;
	m_elements.push_back(e);

	return button;
}

OsuOptionsMenu::OPTIONS_ELEMENT OsuOptionsMenu::addButton(UString text, UString labelText)
{
	OsuUIButton *button = new OsuUIButton(m_osu, 0, 0, m_options->getSize().x, 50, text, text);
	button->setColor(0xff0e94b5);
	button->setUseDefaultSkin();
	m_options->getContainer()->addBaseUIElement(button);

	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 50, "", labelText);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(button);
	e.elements.push_back(label);
	e.type = 4;
	e.cvar = NULL;
	m_elements.push_back(e);

	return e;
}

CBaseUICheckbox *OsuOptionsMenu::addCheckbox(UString text, ConVar *cvar)
{
	CBaseUICheckbox *checkbox = new CBaseUICheckbox(0, 0, m_options->getSize().x, 50, text, text);
	checkbox->setDrawFrame(false);
	checkbox->setDrawBackground(false);
	if (cvar != NULL)
	{
		checkbox->setChecked(cvar->getBool());
		checkbox->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onCheckboxChange) );
	}
	m_options->getContainer()->addBaseUIElement(checkbox);

	OPTIONS_ELEMENT e;
	e.elements.push_back(checkbox);
	e.type = 5;
	e.cvar = cvar;
	m_elements.push_back(e);

	return checkbox;
}

OsuUISlider *OsuOptionsMenu::addSlider(UString text, float min, float max, ConVar *cvar, float label1Width)
{
	OsuUISlider *slider = new OsuUISlider(m_osu, 0, 0, 100, 40, text);
	slider->setAllowMouseWheel(false);
	slider->setBounds(min, max);
	slider->setLiveUpdate(true);
	if (cvar != NULL)
	{
		slider->setValue(cvar->getFloat(), false);
		slider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChange) );
	}
	m_options->getContainer()->addBaseUIElement(slider);

	CBaseUILabel *label1 = new CBaseUILabel(0, 0, m_options->getSize().x, 40, text, text);
	label1->setDrawFrame(false);
	label1->setDrawBackground(false);
	label1->setWidthToContent();
	if (label1Width > 1)
		label1->setSizeX(label1Width);
	m_options->getContainer()->addBaseUIElement(label1);

	CBaseUILabel *label2 = new CBaseUILabel(0, 0, m_options->getSize().x, 40, "", "8.81");
	label2->setDrawFrame(false);
	label2->setDrawBackground(false);
	label2->setWidthToContent();
	m_options->getContainer()->addBaseUIElement(label2);

	OPTIONS_ELEMENT e;
	e.elements.push_back(label1);
	e.elements.push_back(slider);
	e.elements.push_back(label2);
	e.type = 6;
	e.cvar = cvar;
	m_elements.push_back(e);

	return slider;
}

CBaseUITextbox *OsuOptionsMenu::addTextbox(UString text, ConVar *cvar)
{
	CBaseUITextbox *textbox = new CBaseUITextbox(0, 0, m_options->getSize().x, 40, "");
	textbox->setText(text);
	m_options->getContainer()->addBaseUIElement(textbox);

	OPTIONS_ELEMENT e;
	e.elements.push_back(textbox);
	e.type = 7;
	e.cvar = cvar;
	m_elements.push_back(e);

	return textbox;
}

CBaseUIElement *OsuOptionsMenu::addSkinPreview()
{
	CBaseUIElement *skinPreview = new OsuOptionsMenuSkinPreviewElement(m_osu, 0, 0, 0, 200, "");
	m_options->getContainer()->addBaseUIElement(skinPreview);

	OPTIONS_ELEMENT e;
	e.elements.push_back(skinPreview);
	e.type = 8;
	e.cvar = NULL;
	m_elements.push_back(e);

	return skinPreview;
}

CBaseUIElement *OsuOptionsMenu::addSliderPreview()
{
	CBaseUIElement *sliderPreview = new OsuOptionsMenuSliderPreviewElement(m_osu, 0, 0, 0, 200, "");
	m_options->getContainer()->addBaseUIElement(sliderPreview);

	OPTIONS_ELEMENT e;
	e.elements.push_back(sliderPreview);
	e.type = 8;
	e.cvar = NULL;
	m_elements.push_back(e);

	return sliderPreview;
}

void OsuOptionsMenu::save()
{
	if (!osu_options_save_on_back.getBool())
	{
		debugLog("DEACTIVATED SAVE!!!!\n");
		return;
	}

	updateOsuFolder();
	updateName();

	debugLog("Osu: Saving user config file ...\n");

	UString userConfigFile = "cfg/";
	userConfigFile.append(OSU_CONFIG_FILE_NAME);
	userConfigFile.append(".cfg");

	// manual concommands (e.g. fullscreen, windowed, osu_resolution)
	std::vector<ConVar*> manualConCommands;
	std::vector<ConVar*> manualConVars;
	std::vector<ConVar*> removeConCommands;

	removeConCommands.push_back(convar->getConVarByName("windowed"));
	removeConCommands.push_back(convar->getConVarByName("osu_skin"));
	removeConCommands.push_back(convar->getConVarByName("snd_output_device"));

	if (m_fullscreenCheckbox->isChecked())
	{
		manualConCommands.push_back(convar->getConVarByName("fullscreen"));
		if (convar->getConVarByName("osu_resolution_enabled")->getBool())
			manualConVars.push_back(convar->getConVarByName("osu_resolution"));
		else
			removeConCommands.push_back(convar->getConVarByName("osu_resolution"));
	}
	else
	{
		removeConCommands.push_back(convar->getConVarByName("fullscreen"));
		removeConCommands.push_back(convar->getConVarByName("osu_resolution"));
	}

	// get user stuff in the config file
	std::vector<UString> keepLines;
	{
		// in extra block because the File class would block the following std::ofstream from writing to it until it's destroyed
		File in(userConfigFile.toUtf8());
		if (!in.canRead())
			debugLog("Osu Error: Couldn't read user config file!\n");
		else
		{
			while (in.canRead())
			{
				UString uLine = in.readLine();
				const char *lineChar = uLine.toUtf8();
				std::string line(lineChar);

				bool keepLine = true;
				for (int i=0; i<m_elements.size(); i++)
				{
					if (m_elements[i].cvar != NULL && line.find(m_elements[i].cvar->getName().toUtf8()) != std::string::npos)
					{
						keepLine = false;
						break;
					}
				}

				for (int i=0; i<manualConCommands.size(); i++)
				{
					if (line.find(manualConCommands[i]->getName().toUtf8()) != std::string::npos)
					{
						keepLine = false;
						break;
					}
				}

				for (int i=0; i<manualConVars.size(); i++)
				{
					if (line.find(manualConVars[i]->getName().toUtf8()) != std::string::npos)
					{
						keepLine = false;
						break;
					}
				}

				for (int i=0; i<removeConCommands.size(); i++)
				{
					if (line.find(removeConCommands[i]->getName().toUtf8()) != std::string::npos)
					{
						keepLine = false;
						break;
					}
				}

				if (keepLine && line.size() > 0)
					keepLines.push_back(line.c_str());
			}
		}
	}

	// write new config file
	// thankfully this path is relative and hardcoded, and thus not susceptible to unicode characters
	std::ofstream out(userConfigFile.toUtf8());
	if (!out.good())
	{
		engine->showMessageError("Osu Error", "Couldn't write user config file!");
		return;
	}

	// write user stuff back
	for (int i=0; i<keepLines.size(); i++)
	{
		out << keepLines[i].toUtf8() << "\n";
	}
	out << "\n";

	// write manual convars
	for (int i=0; i<manualConCommands.size(); i++)
	{
		out << manualConCommands[i]->getName().toUtf8() << "\n";
	}
	for (int i=0; i<manualConVars.size(); i++)
	{
		out << manualConVars[i]->getName().toUtf8() << " " << manualConVars[i]->getString().toUtf8() <<"\n";
	}

	// hardcoded (!)
	if (engine->getSound()->getOutputDevice() != "Default")
		out << "snd_output_device " << engine->getSound()->getOutputDevice().toUtf8() << "\n";
	if (!m_fullscreenCheckbox->isChecked())
		out << "windowed " << engine->getScreenWidth() << "x" << engine->getScreenHeight() << "\n";

	// write options elements convars
	for (int i=0; i<m_elements.size(); i++)
	{
		if (m_elements[i].cvar != NULL)
		{
			out << m_elements[i].cvar->getName().toUtf8() << " " << m_elements[i].cvar->getString().toUtf8() << "\n";
		}
	}

	out << "osu_skin " << convar->getConVarByName("osu_skin")->getString().toUtf8() << "\n";

	out.close();
}
