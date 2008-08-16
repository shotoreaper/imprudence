/** 
 * @file lluictrlfactory.cpp
 * @brief Factory class for creating UI controls
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2008, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "lluictrlfactory.h"

#include <fstream>
#include <boost/tokenizer.hpp>

// other library includes
#include "llcontrol.h"
#include "lldir.h"
#include "v4color.h"

// this library includes
#include "llbutton.h"
#include "llcheckboxctrl.h"
//#include "llcolorswatch.h"
#include "llcombobox.h"
#include "llcontrol.h"
#include "lldir.h"
#include "llevent.h"
#include "llfloater.h"
#include "lliconctrl.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llradiogroup.h"
#include "llscrollcontainer.h"
#include "llscrollingpanellist.h"
#include "llscrolllistctrl.h"
#include "llslider.h"
#include "llsliderctrl.h"
#include "llmultislider.h"
#include "llmultisliderctrl.h"
#include "llspinctrl.h"
#include "lltabcontainer.h"
#include "lltabcontainervertical.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "llui.h"
#include "llviewborder.h"

const char XML_HEADER[] = "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\" ?>\n";

const S32 HPAD = 4;
const S32 VPAD = 4;
const S32 FLOATER_H_MARGIN = 15;
const S32 MIN_WIDGET_HEIGHT = 10;

std::vector<LLString> LLUICtrlFactory::mXUIPaths;

// UI Ctrl class for padding
class LLUICtrlLocate : public LLUICtrl
{
public:
	LLUICtrlLocate() : LLUICtrl("locate", LLRect(0,0,0,0), FALSE, NULL, NULL) { setTabStop(FALSE); }
	virtual void draw() { }

	static LLView *fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
	{
		LLString name("pad");
		node->getAttributeString("name", name);

		LLUICtrlLocate *new_ctrl = new LLUICtrlLocate();
		new_ctrl->setName(name);
		new_ctrl->initFromXML(node, parent);
		return new_ctrl;
	}
};

static LLRegisterWidget<LLUICtrlLocate> r1("locate");
static LLRegisterWidget<LLUICtrlLocate> r2("pad");

//-----------------------------------------------------------------------------
// LLUICtrlFactory()
//-----------------------------------------------------------------------------
LLUICtrlFactory::LLUICtrlFactory()
	: mDummyPanel(NULL)
{
	setupPaths();
}

LLUICtrlFactory::~LLUICtrlFactory()
{
	delete mDummyPanel;
	mDummyPanel = NULL;
}

void LLUICtrlFactory::setupPaths()
{
	LLString filename = gDirUtilp->getExpandedFilename(LL_PATH_SKINS, "paths.xml");

	LLXMLNodePtr root;
	BOOL success  = LLXMLNode::parseFile(filename, root, NULL);
	mXUIPaths.clear();
	
	if (success)
	{
		LLXMLNodePtr path;
		LLString app_dir = gDirUtilp->getAppRODataDir();
	
		for (path = root->getFirstChild(); path.notNull(); path = path->getNextSibling())
		{
			LLUIString path_val_ui(path->getValue());
			LLString language = "en-us";
			if (LLUI::sConfigGroup)
			{
				language = LLUI::sConfigGroup->getString("Language");
				if(language == "default")
				{
					language = LLUI::sConfigGroup->getString("SystemLanguage");
				}
			}
			path_val_ui.setArg("[Language]", language);
			LLString fullpath = app_dir + path_val_ui.getString();

			if (std::find(mXUIPaths.begin(), mXUIPaths.end(), fullpath) == mXUIPaths.end())
			{
				mXUIPaths.push_back(app_dir + path_val_ui.getString());
			}
		}
	}
	else // parsing failed
	{
		LLString slash = gDirUtilp->getDirDelimiter();
		LLString dir = gDirUtilp->getAppRODataDir() + slash + "skins" + slash + "xui" + slash + "en-us" + slash;
		llwarns << "XUI::config file unable to open." << llendl;
		mXUIPaths.push_back(dir);
	}
}



//-----------------------------------------------------------------------------
// getLayeredXMLNode()
//-----------------------------------------------------------------------------
bool LLUICtrlFactory::getLayeredXMLNode(const LLString &filename, LLXMLNodePtr& root)
{
	if (!LLXMLNode::parseFile(mXUIPaths.front() + filename, root, NULL))
	{	
		if (!LLXMLNode::parseFile(filename, root, NULL))
		{
			llwarns << "Problem reading UI description file: " << mXUIPaths.front() + filename << llendl;
			return FALSE;
		}
	}

	LLXMLNodePtr updateRoot;

	std::vector<LLString>::const_iterator itor;

	for (itor = mXUIPaths.begin(), ++itor; itor != mXUIPaths.end(); ++itor)
	{
		LLString nodeName;
		LLString updateName;

		LLXMLNode::parseFile((*itor) + filename, updateRoot, NULL);
	
		updateRoot->getAttributeString("name", updateName);
		root->getAttributeString("name", nodeName);

		if (updateName == nodeName)
		{
			LLXMLNode::updateNode(root, updateRoot);
		}
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// buildFloater()
//-----------------------------------------------------------------------------
void LLUICtrlFactory::buildFloater(LLFloater* floaterp, const LLString &filename, 
									const LLCallbackMap::map_t* factory_map, BOOL open) /* Flawfinder: ignore */
{
	LLXMLNodePtr root;

	if (!LLUICtrlFactory::getLayeredXMLNode(filename, root))
	{
		return;
	}
	
	// root must be called floater
	if( !(root->hasName("floater") || root->hasName("multi_floater") ) )
	{
		llwarns << "Root node should be named floater in: " << filename << llendl;
		return;
	}

	if (factory_map)
	{
		mFactoryStack.push_front(factory_map);
	}

	floaterp->initFloaterXML(root, NULL, this, open);	/* Flawfinder: ignore */

	if (LLUI::sShowXUINames)
	{
		floaterp->setToolTip(filename);
	}

	if (factory_map)
	{
		mFactoryStack.pop_front();
	}

	LLHandle<LLFloater> handle = floaterp->getHandle();
	mBuiltFloaters[handle] = filename;
}

//-----------------------------------------------------------------------------
// saveToXML()
//-----------------------------------------------------------------------------
S32 LLUICtrlFactory::saveToXML(LLView* viewp, const LLString& filename)
{
	llofstream out(filename.c_str());
	if (!out.good())
	{
		llwarns << "Unable to open " << filename << " for output." << llendl;
		return 1;
	}

	out << XML_HEADER;

	LLXMLNodePtr xml_node = viewp->getXML();

	xml_node->writeToOstream(out);

	out.close();
	return 0;
}

//-----------------------------------------------------------------------------
// buildPanel()
//-----------------------------------------------------------------------------
BOOL LLUICtrlFactory::buildPanel(LLPanel* panelp, const LLString &filename,
									const LLCallbackMap::map_t* factory_map)
{
	BOOL didPost = FALSE;
	LLXMLNodePtr root;

	if (!LLUICtrlFactory::getLayeredXMLNode(filename, root))
	{
		return didPost;
	}

	// root must be called panel
	if( !root->hasName("panel" ) )
	{
		llwarns << "Root node should be named panel in : " << filename << llendl;
		return didPost;
	}

	if (factory_map)
	{
		mFactoryStack.push_front(factory_map);
	}

	didPost = panelp->initPanelXML(root, NULL, this);
	
	if (LLUI::sShowXUINames)
	{
		panelp->setToolTip(filename);
	}

	LLHandle<LLPanel> handle = panelp->getHandle();
	mBuiltPanels[handle] = filename;

	if (factory_map)
	{
		mFactoryStack.pop_front();
	}

	return didPost;
}

//-----------------------------------------------------------------------------
// buildMenu()
//-----------------------------------------------------------------------------
LLMenuGL *LLUICtrlFactory::buildMenu(const LLString &filename, LLView* parentp)
{
	// TomY TODO: Break this function into buildMenu and buildMenuBar
	LLXMLNodePtr root;
	LLMenuGL*    menu;

	if (!LLUICtrlFactory::getLayeredXMLNode(filename, root))
	{
		return NULL;
	}

	// root must be called panel
	if( !root->hasName( "menu_bar" ) && !root->hasName( "menu" ))
	{
		llwarns << "Root node should be named menu bar or menu in : " << filename << llendl;
		return NULL;
	}

	if (root->hasName("menu"))
	{
		menu = (LLMenuGL*)LLMenuGL::fromXML(root, parentp, this);
	}
	else
	{
		menu = (LLMenuGL*)LLMenuBarGL::fromXML(root, parentp, this);
	}
	
	if (LLUI::sShowXUINames)
	{
		menu->setToolTip(filename);
	}

    return menu;
}

//-----------------------------------------------------------------------------
// buildMenu()
//-----------------------------------------------------------------------------
LLPieMenu *LLUICtrlFactory::buildPieMenu(const LLString &filename, LLView* parentp)
{
	LLXMLNodePtr root;

	if (!LLUICtrlFactory::getLayeredXMLNode(filename, root))
	{
		return NULL;
	}

	// root must be called panel
	if( !root->hasName( LL_PIE_MENU_TAG ))
	{
		llwarns << "Root node should be named " << LL_PIE_MENU_TAG << " in : " << filename << llendl;
		return NULL;
	}

	LLString name("menu");
	root->getAttributeString("name", name);

	LLPieMenu *menu = new LLPieMenu(name);
	parentp->addChild(menu);
	menu->initXML(root, parentp, this);

	if (LLUI::sShowXUINames)
	{
		menu->setToolTip(filename);
	}

	return menu;
}

//-----------------------------------------------------------------------------
// rebuild()
//-----------------------------------------------------------------------------
void LLUICtrlFactory::rebuild()
{
	built_panel_t::iterator built_panel_it;
	for (built_panel_it = mBuiltPanels.begin();
		built_panel_it != mBuiltPanels.end();
		++built_panel_it)
	{
		LLString filename = built_panel_it->second;
		LLPanel* panelp = built_panel_it->first.get();
		if (!panelp)
		{
			continue;
		}
		llinfos << "Rebuilding UI panel " << panelp->getName() 
			<< " from " << filename
			<< llendl;
		BOOL visible = panelp->getVisible();
		panelp->setVisible(FALSE);
		panelp->setFocus(FALSE);
		panelp->deleteAllChildren();

		buildPanel(panelp, filename.c_str(), &panelp->getFactoryMap());
		panelp->setVisible(visible);
	}

	built_floater_t::iterator built_floater_it;
	for (built_floater_it = mBuiltFloaters.begin();
		built_floater_it != mBuiltFloaters.end();
		++built_floater_it)
	{
		LLFloater* floaterp = built_floater_it->first.get();
		if (!floaterp)
		{
			continue;
		}
		LLString filename = built_floater_it->second;
		llinfos << "Rebuilding UI floater " << floaterp->getName()
			<< " from " << filename
			<< llendl;
		BOOL visible = floaterp->getVisible();
		floaterp->setVisible(FALSE);
		floaterp->setFocus(FALSE);
		floaterp->deleteAllChildren();

		gFloaterView->removeChild(floaterp);
		buildFloater(floaterp, filename, &floaterp->getFactoryMap());
		floaterp->setVisible(visible);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

LLView *LLUICtrlFactory::createCtrlWidget(LLPanel *parent, LLXMLNodePtr node)
{
	LLString ctrl_type = node->getName()->mString;
	LLString::toLower(ctrl_type);
	
	LLWidgetClassRegistry::factory_func_t func = LLWidgetClassRegistry::getInstance()->getCreatorFunc(ctrl_type);

	if (func == NULL)
	{
		llwarns << "Unknown control type " << ctrl_type << llendl;
		return NULL;
	}

	if (parent == NULL)
	{
		if (mDummyPanel == NULL)
		{
			mDummyPanel = new LLPanel;
		}
		parent = mDummyPanel;
	}
	LLView *ctrl = func(node, parent, this);

	return ctrl;
}

LLView* LLUICtrlFactory::createWidget(LLPanel *parent, LLXMLNodePtr node)
{
	LLView* view = createCtrlWidget(parent, node);

	S32 tab_group = parent->getLastTabGroup();
	node->getAttributeS32("tab_group", tab_group);

	if (view)
	{
		parent->addChild(view, tab_group);
	}

	return view;
}

//-----------------------------------------------------------------------------
// createFactoryPanel()
//-----------------------------------------------------------------------------
LLPanel* LLUICtrlFactory::createFactoryPanel(LLString name)
{
	std::deque<const LLCallbackMap::map_t*>::iterator itor;
	for (itor = mFactoryStack.begin(); itor != mFactoryStack.end(); ++itor)
	{
		const LLCallbackMap::map_t* factory_map = *itor;

		// Look up this panel's name in the map.
		LLCallbackMap::map_const_iter_t iter = factory_map->find( name );
		if (iter != factory_map->end())
		{
			// Use the factory to create the panel, instead of using a default LLPanel.
			LLPanel *ret = (LLPanel*) iter->second.mCallback( iter->second.mData );
			return ret;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------

//static
BOOL LLUICtrlFactory::getAttributeColor(LLXMLNodePtr node, const LLString& name, LLColor4& color)
{
	LLString colorstring;
	BOOL res = node->getAttributeString(name, colorstring);
	if (res && LLUI::sColorsGroup)
	{
		if (LLUI::sColorsGroup->controlExists(colorstring))
		{
			color.setVec(LLUI::sColorsGroup->getColor(colorstring));
		}
		else
		{
			res = FALSE;
		}
	}
	if (!res)
	{
		res = LLColor4::parseColor(colorstring.c_str(), &color);
	}	
	if (!res)
	{
		res = node->getAttributeColor(name, color);
	}
	return res;
}

