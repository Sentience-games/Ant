#pragma once

#include "ant_shared.h"
#include "immediate/immediate.h"
#include "math/vector.h"
#include "utils/memory_utils.h"

#define UI_PANEL_TITLEBAR_HEIGHT    0.256f
#define UI_PANEL_INDENTATION_AMOUNT 0.128f
#define UI_CATEGORY_OPEN_SPEED      0.001f

enum UI_PANEL_ID
{
	UIPanel_Debug,
    
	UI_PANEL_COUNT
};

enum UI_ELEMENT_TYPE
{
	UIElement_NOTYPE,
	UIElement_Button,
	UIElement_CategoryButton
};

enum UI_ELEMENT_WIDTH
{
	UIElementWidth_Eighth  = 125,
	UIElementWidth_Quarter = 250,
	UIElementWidth_Third   = 333,
	UIElementWidth_Half	= 500,
	UIElementWidth_Full	= 1000,
};

struct ui_element
{
	ui_element* next;
    
	enum32(UI_ELEMENT_TYPE) type;
	enum32(UI_PANEL_ID) owner;
	u32 index;
	const char* label;
    
	v2 position;
	v2 dimensions;
};

struct ui_panel
{
	ui_element* first_element;
	ui_element* current_element;
    
	enum32(UI_PANEL_ID) id;
    
	v2 position;
	v2 dimensions;
	f32 row_height;
	v2 at;
    
	u32 num_elements;
    
	u32 selected_index;
	u32 current_indentation;
    
	const char* label;
    
    bool is_occluded;
    
    // Used by panel movement
    v2 move_anchor;
    bool is_moving;
    bool is_initialized;
};

struct ui_occlusion
{
    ui_occlusion* next;
    
    v2 position;
    v2 dimensions;
};

global_variable
struct ui_state
{
	ui_panel* hot_panel;
    
	ui_element hot;
	ui_element active;
    
	ui_element* selected;
    
	bool selection_mode;
    
	ui_panel* current_panel;
	ui_panel panels[UI_PANEL_COUNT];
    
    ui_occlusion* occlusions;
    bool is_occlusion_list_being_built;
} UIState;

inline bool
UIIsHit(const v2& position, const v2& dimensions)
{
	bool result = false;
    
	v2 mouse = NewInput->mouse.position;
    
	if ((position.x <= mouse.x && mouse.x <= position.x + dimensions.x)
		&& (position.y <= mouse.y && mouse.y <= position.y + dimensions.y))
	{
		result = true;
	}
    
	return result;
}

inline void
UIDeclareOcclusion(v2 position, v2 dimensions)
{
    ui_occlusion* occlusion = NULL;
    
    // If there is no hot panel or the hot panel is reset or empty the occlusion list should be cleared
    if (!UIState.is_occlusion_list_being_built
        && (UIState.hot_panel == NULL || UIState.hot_panel->num_elements == 0))
    {
        UIState.is_occlusion_list_being_built = true;
        
        occlusion = PushStruct(FrameTempArena, ui_occlusion);
        UIState.occlusions = occlusion;
    }
    
    else
    {
        ui_occlusion* current_occlusion = occlusion;
        
        while(current_occlusion && current_occlusion->next)
        {
            current_occlusion = current_occlusion->next;
        }
        
        occlusion               = PushStruct(FrameTempArena, ui_occlusion);
        current_occlusion->next = occlusion;
    }
    
    occlusion->position   = position;
    occlusion->dimensions = dimensions;
}

#define UI_ELEMENTS_EQUAL(e_1, e_2)\
(e_1)->type     != UIElement_NOTYPE\
&& (e_2)->type  != UIElement_NOTYPE\
&& (e_1)->owner == (e_2)->owner\
&& (e_1)->type  == (e_2)->type\
&& (e_1)->index == (e_2)->index\
&& (e_2)->label == (e_2)->label

inline void
UIBeginPanel(enum32(UI_PANEL_ID) id, const char* label, v2 position, f32 width, f32 row_height)
{
	Assert(UIState.current_panel == NULL);
    
    // FIXME(soimn): this is kind of ugly
    UIState.is_occlusion_list_being_built = false;
    
	ui_panel* panel	   = &UIState.panels[id];
	UIState.current_panel = panel;
    
	panel->id		 = id;
	panel->row_height = UI_PANEL_TITLEBAR_HEIGHT;
	panel->dimensions = {width, MAX(panel->dimensions.y, panel->row_height)};
	panel->at		 = {0.0f, panel->row_height};
    
    // TODO(soimn): consider refactor
    if (!panel->is_initialized)
    {
        panel->position       = position;
        panel->is_initialized = true;
    }
    
	v4 titlebar_color = {0.78f, 0.42f, 0.21f, 1.00f};
    
	if (UIState.hot_panel != panel)
	{
		titlebar_color *= 0.2f;
	}
    
	PushIMQuad(panel->position, {panel->dimensions.x, panel->row_height}, titlebar_color, ImmediateLayer_UI_Foreground);
}

inline void
UIEndPanel()
{
	Assert(UIState.current_panel != NULL);
    
	ui_panel* panel	   = UIState.current_panel;
	UIState.current_panel = NULL;
    
	panel->dimensions.y = MAX(panel->at.y + panel->row_height, panel->row_height);
    
	v4 background_color = {0.22f, 0.22f, 0.22f, 1.0f};
	PushIMQuad(panel->position, panel->dimensions, background_color, ImmediateLayer_UI_Background);
    
	if (UIState.hot.type == UIElement_NOTYPE)
	{
		UIState.hot	= {};
		UIState.active = {};
	}
    
	v2 mouse = NewInput->mouse.position;
	if (UIIsHit(panel->position, panel->dimensions)
		&& (!UIState.selection_mode || UIState.hot_panel == panel))
	{
        ui_occlusion* occlusion = UIState.occlusions;
        
        while (occlusion)
        {
            if (UIIsHit(occlusion->position, occlusion->dimensions))
            {
                panel->is_occluded = true;
            }
            
            occlusion = occlusion->next;
        }
        
        if (!panel->is_occluded)
        {
            UIState.hot_panel = panel;
            
            if (!panel->is_moving 
                && UIIsHit(panel->position, {panel->dimensions.x, panel->row_height}) 
                && (!OldInput->mouse.left && NewInput->mouse.left))
            {
                panel->move_anchor = panel->position - NewInput->mouse.position;
                panel->is_moving   = true;
            }
            
            // Check for the tab key being pressed and enumerate the list of elements, looking for the last active element, if found the
            // succeeding element is set to the selected state, if the element is not found, the first element is set instead
            if (!OldInput->tab && NewInput->tab)
            {
                if (UIState.selected == NULL || UIState.selected->index >= panel->num_elements)
                {
                    UIState.selected = panel->first_element;
                }
                
                else
                {
                    UIState.selected = UIState.selected->next;
                    ++panel->selected_index;
                    
                    if (UIState.selected == NULL)
                    {
                        UIState.selected	  = panel->first_element;
                        panel->selected_index = 0;
                    }
                }
            }
            
            if (UIState.selection_mode)
            {
                Assert(UIState.selected);
                PushIMQuad(UIState.selected->position, UIState.selected->dimensions, {1.0f, 1.0f, 0.0f, 1.0f}, ImmediateLayer_UI_Foreground);
            }
        }
        
        else
        {
            if (UIState.hot_panel == panel)
            {
                UIState.hot_panel = NULL;
                UIState.hot       = {};
                UIState.active    = {};
                
                panel->is_moving = false;
            }
        }
    }
    
	else if (UIState.hot_panel == panel && !panel->is_moving)
	{
		UIState.hot_panel = NULL;
		UIState.hot	   = {};
        UIState.active    = {};
	}
    
    if (panel->is_moving)
    {
        if (NewInput->mouse.left)
        {
            panel->position = NewInput->mouse.position + panel->move_anchor;
        }
        
        else
        {
            panel->is_moving = false;
        }
    }
    
	panel->num_elements        = 0;
    panel->is_occluded         = false;
}

inline void
UINewRow()
{
	Assert(UIState.current_panel != NULL);
	UIState.current_panel->at.x  = 0.0f;
	UIState.current_panel->at.y += UIState.current_panel->row_height;
}

inline void
UINewIndent(u32 amount = 1)
{
	Assert(UIState.current_panel != NULL);
	UIState.current_panel->current_indentation += amount;
}

inline void
UIRemoveIndent(u32 amount = 1)
{
	Assert(UIState.current_panel != NULL);
	Assert(UIState.current_panel->current_indentation >= amount);
    
    ui_panel* panel = UIState.current_panel;
    
	panel->current_indentation -= amount;
}

inline void
UIMakeNewElement_(ui_panel* panel, ui_element* element, enum32(UI_ELEMENT_WIDTH) element_width)
{
	element->owner = panel->id;
	element->index = panel->num_elements++;
    
	f32 indent_amount = (f32) panel->current_indentation * UI_PANEL_INDENTATION_AMOUNT;
	panel->at.x += indent_amount;
    
	element->position   = panel->position + panel->at;
	element->dimensions = {panel->dimensions.x * ((f32) element_width / 1000.0f) - indent_amount,
        panel->row_height};
    
	panel->at.x += element->dimensions.x;
    
	if (!panel->first_element)
	{
		panel->first_element = element;
	}
    
	else
	{
		panel->current_element->next = element;
	}
    
	panel->current_element = element;
    
	/// Tabbing
	if (element->index == panel->selected_index)
	{
		UIState.selected = element;
	}
}

inline void
UIButtonLikeActiveBehaviour_(ui_panel* panel, ui_element* element)
{
	if (UIIsHit(element->position, element->dimensions)
		&& !panel->is_occluded)
	{
		if (UIState.active.type == UIElement_NOTYPE)
		{
			UIState.hot = *element;
            
			if (!OldInput->mouse.left && NewInput->mouse.left)
			{
				UIState.active = *element;
			}
		}
	}
    
	else if (UI_ELEMENTS_EQUAL(&UIState.active, element))
	{
		UIState.hot	   = {};
		UIState.active = {};
	}
}

inline bool
UIButton(const char* label, enum32(UI_ELEMENT_WIDTH) element_width)
{
	Assert(UIState.current_panel != NULL);
	ui_panel* panel = UIState.current_panel;
    
	bool result = false;
    
	ui_element* element = PushStruct(FrameTempArena, ui_element);
	element->type	   = UIElement_Button;
    
	UIMakeNewElement_(panel, element, element_width);
	UIButtonLikeActiveBehaviour_(panel, element);
    
	v4 color = {0.44f, 0.44f, 0.44f, 1.0f};
	if (UI_ELEMENTS_EQUAL(&UIState.active, element))
		color *= 0.5f;
    
	if (UI_ELEMENTS_EQUAL(&UIState.active, element) && !NewInput->mouse.left)
	{
		if (UI_ELEMENTS_EQUAL(&UIState.hot, element))
		{
			result = true;
		}
        
		UIState.active = {};
	}
    
	PushIMQuad(element->position, element->dimensions, color, ImmediateLayer_UI_Foreground);
    
	return result;
}

inline bool
UICategoryButton(const char* label, enum32(UI_ELEMENT_WIDTH) element_width, bool* is_open)
{
	Assert(UIState.current_panel != NULL);
	ui_panel* panel = UIState.current_panel;
    
	Assert(is_open != NULL);
    
	ui_element* element = PushStruct(FrameTempArena, ui_element);
	element->type	   = UIElement_CategoryButton;
    
	UIMakeNewElement_(panel, element, element_width);
	UIButtonLikeActiveBehaviour_(panel, element);
    
	v4 color = {0.44f, 0.44f, 0.44f, 1.0f};
	if (UI_ELEMENTS_EQUAL(&UIState.active, element))
		color *= 0.5f;
    
	if (UI_ELEMENTS_EQUAL(&UIState.active, element) && !NewInput->mouse.left)
	{
		if (UI_ELEMENTS_EQUAL(&UIState.hot, element))
		{
			*is_open = !*is_open;
		}
        
		UIState.active = {};
	}
    
	PushIMQuad(element->position, element->dimensions, color, ImmediateLayer_UI_Foreground);
    
	return *is_open;
}
