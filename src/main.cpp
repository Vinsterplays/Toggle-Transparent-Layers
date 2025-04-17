#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/utils/cocos.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <cvolton.level-id-api/include/EditorIDs.hpp>
#include <Geode/modify/GameObject.hpp>
class $modify(LayerButtonUI, EditorUI)
{
	bool init(LevelEditorLayer *p0)
	{
		if (!EditorUI::init(p0))
			return false;
		auto fields = m_fields.self();
		fields->levelId = EditorIDs::getID(p0->m_level);
		fields->loadMap();
		if (auto layerMenu = this->getChildByID("layer-menu"))
		{
			auto layerHideSpr = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
			auto layerShowSpr = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
			m_fields->layerToggle = CCMenuItemToggler::create(
				layerHideSpr, layerShowSpr, this, menu_selector(LayerButtonUI::onToggle));
			fields->layerToggle->setID("hide-layer-toggle"_spr);
			fields->layerToggle->m_notClickable = true;
			fields->layerToggle->toggle(true);
			layerMenu->addChild(fields->layerToggle);
			layerMenu->updateLayout();
			updateLayer(p0->m_currentLayer);
			schedule(schedule_selector(LayerButtonUI::checkLayer), 0);
		}
		return true;
	}
	void updateLayer(int layer)
	{
		auto fields = m_fields.self();
		auto it = fields->hiddenLayers.find(layer);
		if (fields->layerToggle)
		{
			if (it != fields->hiddenLayers.end())
			{
				fields->layerToggle->toggle(false);
			}
			else
			{

				fields->layerToggle->toggle(true);
			}
		}
		int currentLayer = m_editorLayer->m_currentLayer;

		fields->layerToggle->setVisible(currentLayer != -1);

		for (auto *obj : CCArrayExt<GameObject *>(this->m_editorLayer->m_objects))
		{
			if (!obj)
				continue;

			int l1 = obj->m_editorLayer;
			int l2 = obj->m_editorLayer2;

			if (currentLayer == -1 ||
				l1 == currentLayer ||
				(l2 == currentLayer && l2 != 0) ||
				obj->m_isSelected)
			{
				if (!obj->isVisible())
					obj->setVisible(true);
				continue;
			}

			bool layer1Hidden = fields->hiddenLayers.contains(l1);
			bool layer2Hidden = (l2 != 0) ? fields->hiddenLayers.contains(l2) : true;

			if (layer1Hidden && layer2Hidden)
			{
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
		auto fields = m_fields.self();
		if (m_fields->layerToggle)
		{
			if (fields->hiddenLayers.contains(layer))
			{
				fields->hiddenLayers.erase(layer);
				fields->layerToggle->toggle(true);
			}
			else
			{
				fields->hiddenLayers[layer] = true;
				fields->layerToggle->toggle(false);
			}
		}
	}
	void showUI(bool show)
	{
		auto fields = m_fields.self();
		if (m_editorLayer->m_currentLayer != -1)
		{
			fields->layerToggle->setVisible(show);
		}
		else
		{
			fields->layerToggle->setVisible(false);
		}
		EditorUI::showUI(show);
	}
	void onPlaytest(CCObject *sender)
	{
		EditorUI::onPlaytest(sender);
		auto fields = m_fields.self();
		for (auto *obj : CCArrayExt<GameObject *>(m_editorLayer->m_objects))
		{
			if (!obj)
				continue;

			int l1 = obj->m_editorLayer;
			int l2 = obj->m_editorLayer2;

			bool layer1Hidden = fields->hiddenLayers.contains(l1);
			bool layer2Hidden = (l2 != 0) ? fields->hiddenLayers.contains(l2) : true;

			if (layer1Hidden && layer2Hidden)
			{
				obj->setVisible(true);
			}
		}
	}
	void onStopPlaytest(CCObject *sender)
	{
		updateLayer(m_editorLayer->m_currentLayer);
		EditorUI::onStopPlaytest(sender);
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
class $modify(GameObject)
{
	void setVisible(bool p0)
	{
		bool visible = this->isVisible();
		GameObject::setVisible(p0);
		if (visible == false && p0 == true)
		{
			if (auto ui = EditorUI::get())
			{
				if (auto myUI = static_cast<LayerButtonUI *>(ui))
				{
					auto fields = myUI->m_fields.self();
					if (ui->m_editorLayer->m_currentLayer == -1 ||
						this->m_objectID == 0 ||
						ui->m_editorLayer->m_playbackMode == PlaybackMode::Playing ||
						this->m_editorLayer == ui->m_editorLayer->m_currentLayer ||
						(this->m_editorLayer2 == ui->m_editorLayer->m_currentLayer && this->m_editorLayer2 != 0) ||
						this->m_isSelected)
					{
						return;
					}
					bool layer1Hidden = fields->hiddenLayers.contains(this->m_editorLayer);
					bool layer2Hidden = (this->m_editorLayer2 != 0) ? fields->hiddenLayers.contains(this->m_editorLayer2) : true;

					if (layer1Hidden && layer2Hidden)
					{
						this->setVisible(false);
						return;
					}
				}
			}
		}
	}
};