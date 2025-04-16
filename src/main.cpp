#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <cvolton.level-id-api/include/EditorIDs.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
class $modify(LayerButtonUI, EditorUI)
{
	bool init(LevelEditorLayer *p0)
	{
		if (!EditorUI::init(p0))
			return false;
		m_fields->levelId = EditorIDs::getID(p0->m_level);
		m_fields->loadMap();
		if (auto layerMenu = this->getChildByID("layer-menu"))
		{
			auto layerHideSpr = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
			auto layerShowSpr = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
			m_fields->layerToggle = CCMenuItemToggler::create(
				layerHideSpr, layerShowSpr, this, menu_selector(LayerButtonUI::onToggle));
			m_fields->layerToggle->setID("hide-layer-toggle"_spr);
			m_fields->layerToggle->m_notClickable = true;
			m_fields->layerToggle->toggle(true);
			layerMenu->addChild(m_fields->layerToggle);
			layerMenu->updateLayout();
			updateLayer(p0->m_currentLayer);
			schedule(schedule_selector(LayerButtonUI::checkLayer), 0);
		}
		else
		{
			log::warn("LayerButtonUI: layer-menu not found!");
		}
		return true;
	}
	void updateLayer(int layer)
	{
		auto it = m_fields->hiddenLayers.find(layer);
		if (it != m_fields->hiddenLayers.end())
		{
			if (m_fields->layerToggle)
				m_fields->layerToggle->toggle(false);
		}
		else
		{
			if (m_fields->layerToggle)
				m_fields->layerToggle->toggle(true);
		}
		auto fields = m_fields.self();
		int currentLayer = m_editorLayer->m_currentLayer;

		fields->layerToggle->setVisible(m_editorLayer->m_playbackMode != PlaybackMode::Playing && currentLayer != -1);
		bool isPlaying = m_editorLayer->m_playbackMode == PlaybackMode::Playing;

		for (auto *obj : CCArrayExt<GameObject *>(this->m_editorLayer->m_objects))
		{
			if (!obj)
				continue;

			int l1 = obj->m_editorLayer;
			int l2 = obj->m_editorLayer2;

			// Quick check if we can skip (object is always visible)
			if (currentLayer == -1 ||
				l1 == currentLayer ||
				(l2 == currentLayer && l2 != 0) ||
				obj->m_isSelected)
			{
				if (!obj->isVisible())
					obj->setVisible(true);
				continue;
			}

			// If both layers are hidden
			bool layer1Hidden = fields->hiddenLayers.contains(l1);
			bool layer2Hidden = (l2 != 0) ? fields->hiddenLayers.contains(l2) : true;

			if (layer1Hidden && layer2Hidden && !isPlaying)
			{
				// Only set to false if not already hidden
				if (obj->isVisible())
					obj->setVisible(false);
			}
		}
	}
	void checkLayer(float)
	{
		static int layer = -500;
		if (m_editorLayer->m_currentLayer == layer)
			return;
		layer = m_editorLayer->m_currentLayer;
		updateLayer(layer);
	}
	void onToggle(CCObject *)
	{
		int layer = m_editorLayer->m_currentLayer;

		if (m_fields->hiddenLayers.contains(layer))
		{
			// Toggling ON (make visible)
			m_fields->hiddenLayers.erase(layer);
			if (m_fields->layerToggle)
				m_fields->layerToggle->toggle(true);
		}
		else
		{
			// Toggling OFF (make hidden)
			m_fields->hiddenLayers[layer] = true;
			if (m_fields->layerToggle)
				m_fields->layerToggle->toggle(false);
		}
	}
	struct Fields
	{
		CCMenuItemToggler *layerToggle = nullptr;
		int levelId = 0;
		std::unordered_map<int, bool> hiddenLayers;

		void loadMap()
		{
			if (auto res = matjson::parse(Mod::get()->getSavedValue<std::string>(std::to_string(levelId), "{}")))
			{
				for (auto &[key, value] : *res)
				{
					if (value.isBool() && value.asBool())
					{
						hiddenLayers[std::atoi(key.c_str())] = true;
					}
				}
			}
		}
		~Fields()
		{
			matjson::Value jsonVal;
			for (const auto &[key, value] : hiddenLayers)
			{
				if (value)
					jsonVal[std::to_string(key)] = true;
			}
			Mod::get()->setSavedValue(std::to_string(levelId), jsonVal.dump(matjson::NO_INDENTATION));
		}
	};
};