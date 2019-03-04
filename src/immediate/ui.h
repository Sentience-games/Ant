#pragma once

#include "ant_shared.h"
#include "immediate/immediate.h"
#include "math/vector.h"
#include "math/interpolation.h"
#include "utils/memory_utils.h"

#define UI_PANEL_TITLEBAR_HEIGHT  0.256f // 0.5 / 35 of the screen
#define UI_PANEL_MIN_EMPTY_WIDTH  5.0f
#define UI_PANEL_MIN_EMPTY_HEIGHT 5.0f

enum UI_PANEL_ID
{
	UIPanel_NOID,

	// All panel ids go below this line
	
	UIPanel_Debug,
	UIPanel_Debug2,

	// end of panel ids

	UI_PANEL_COUNT
};

enum UI_REGION_SPECIFIER
{
	UIRegion_Center = BITS(1),
	UIRegion_Left	= BITS(2),
	UIRegion_Right	= BITS(3),
	UIRegion_Top	= BITS(4),
	UIRegion_Bottom = BITS(5)
};

enum UI_ELEMENT_TYPE
{

	UI_ELEMENT_TYPE_COUNT
};

enum UI_COLOR_VALUE
{
	UIColor_Default,

	UIColor_PanelBase,
	UIColor_PanelTitlebar,
	UIColor_PanelTitlebarInactive,
	UIColor_Button,

	UI_COLORS_COUNT
};

struct ui_element
{
	ui_element* next;

	enum32(UI_ELEMENT_TYPE) type;
	v2 position;
	v2 dimensions;

	union
	{
	};
};

struct ui_panel
{
	ui_element* first_element;

	enum32(UI_PANEL_ID) id;
	v2 position;
	v2 dimensions;

	v2 current_anchor;

	b32 is_visible;
	u32 depth;

	const char* label;
};

struct ui_occlusion_rect
{
	ui_occlusion_rect* next;
	v2 position;
	v2 dimensions;
};

v4 ui_colors[UI_COLORS_COUNT] = {
	{1.00f, 0.00f, 1.00f, 1.00f},
	{0.22f, 0.22f, 0.22f, 1.00f},
	{0.78f, 0.42f, 0.21f, 1.00f},
	{0.35f, 0.35f, 0.35f, 1.00f},
	{0.78f, 0.42f, 0.21f, 1.00f}
};

global_variable
struct ui_state
{
	ui_element hot_element;
	ui_element active_element;
	ui_panel*  hot_panel;
	ui_panel*  active_panel;

	u32 highest_depth_value;
	ui_panel panels[UI_PANEL_COUNT];

	ui_occlusion_rect* occlusions;
} UIState;

inline bool
UIHitTest(v2 position, v2 dimensions)
{
	bool result = false;
	v2 mouse	= NewInput->mouse.position;

	if ((position.x <= mouse.x && mouse.x <= position.x + dimensions.x)
		&& (position.y <= mouse.y && mouse.y <= position.y + dimensions.y))
	{
		result = true;
	}

	return result;
}

inline void
UIDrawPanel_(ui_panel* panel)
{
	v4 titlebar_color = ui_colors[UIColor_PanelTitlebar];

	if (UIState.hot_panel != panel)
	{
		titlebar_color = ui_colors[UIColor_PanelTitlebarInactive];
	}

	PushIMQuad(panel->position, panel->dimensions, ui_colors[UIColor_PanelBase]);
	PushIMQuad(panel->position, {panel->dimensions.x, UI_PANEL_TITLEBAR_HEIGHT}, titlebar_color);
}

inline void
UIDrawPanelElement_(ui_panel* panel, ui_element* element)
{

}

inline void
UIRender()
{
	u32 ordered_panel_count					 = 0;
	ui_panel* ordered_panels[UI_PANEL_COUNT] = {};

	ui_occlusion_rect* occlusion = UIState.occlusions;

	while(occlusion)
	{
		v2 mouse = NewInput->mouse.position;

		if ((occlusion->position.x <= mouse.x && mouse.x <= occlusion->position.x + occlusion->dimensions.x)
			&& (occlusion->position.y <= mouse.y && mouse.y <= occlusion->position.y + occlusion->dimensions.y))
		{
			UIState.active_panel = NULL;
			UIState.hot_panel	 = NULL;

// 			UIState.active_element = NULL;
// 			UIState.hot_element    = NULL;
		}

		occlusion = occlusion->next;
	}


	for (u32 i = UIPanel_NOID + 1; i < UI_PANEL_COUNT; ++i)
	{
		ui_panel* panel = &UIState.panels[i];

		if (!panel->is_visible)
		{
			continue;
		}

		else
		{
			u32 insertion_index = 0;

			for (; insertion_index < ordered_panel_count; ++insertion_index)
			{
				if (panel->depth < ordered_panels[insertion_index]->depth)
				{
					break;
				}
			}

			CopyBufferedArray(FrameTempArena, ordered_panels, ordered_panel_count - insertion_index, ordered_panels + insertion_index + 1);
			ordered_panels[insertion_index] = panel;
			++ordered_panel_count;
		}
	}

	for (u32 i = 0; i < ordered_panel_count; ++i)
	{
		UIDrawPanel_(ordered_panels[i]);

		ui_element* next_element = ordered_panels[i]->first_element;

		while (next_element)
		{
			UIDrawPanelElement_(ordered_panels[i], next_element);

			next_element = next_element->next;
		}
	}

	UIState.hot_panel = NULL;
}

// TODO(soimn): spawn position
inline void
UIPanel(enum32(UI_PANEL_ID) id, u8 region_flag, const char* label)
{
	Assert(id < UI_PANEL_COUNT);

	ui_panel* panel = &UIState.panels[id];

	if (!panel->is_visible)
	{
		panel->id		  = id;
		panel->label	  = label;
		panel->dimensions = {UI_PANEL_MIN_EMPTY_WIDTH, UI_PANEL_MIN_EMPTY_HEIGHT};
		panel->is_visible = true;

		if (region_flag & UIRegion_Center)
		{
			if (region_flag == UIRegion_Center)
			{
				panel->position = (VulkanState->render_target.immediate_viewport_dimensions - panel->dimensions) / 2.0f;
			}

			else if (region_flag & UIRegion_Right)
			{
				panel->position.x = VulkanState->render_target.immediate_viewport_dimensions.x  - panel->dimensions.x;
				panel->position.y = (VulkanState->render_target.immediate_viewport_dimensions.y - panel->dimensions.y) / 2.0f;
			}

			else if (region_flag & UIRegion_Top)
			{
				panel->position.x = (VulkanState->render_target.immediate_viewport_dimensions.x - panel->dimensions.x) / 2.0f;
				panel->position.y = 0.0f;
			}

			else if (region_flag & UIRegion_Bottom)
			{
				panel->position.x = (VulkanState->render_target.immediate_viewport_dimensions.x - panel->dimensions.x) / 2.0f;
				panel->position.y = VulkanState->render_target.immediate_viewport_dimensions.y  - panel->dimensions.y;
			}

			else
			{
				INVALID_CODE_PATH;
			}
		}

		else
		{
			Assert(!((region_flag & UIRegion_Left) && (region_flag & UIRegion_Right)));
			Assert(!((region_flag & UIRegion_Top)  && (region_flag & UIRegion_Bottom)));

			if (region_flag & UIRegion_Left)
			{
				panel->position.x = 0.0f;
			}

			else if (region_flag & UIRegion_Right)
			{
				panel->position.x = VulkanState->render_target.immediate_viewport_dimensions.x - panel->dimensions.x;
			}

			if (region_flag & UIRegion_Top)
			{
				panel->position.y = 0.0f;
			}

			else if (region_flag & UIRegion_Bottom)
			{
				panel->position.y = VulkanState->render_target.immediate_viewport_dimensions.y - panel->dimensions.y;
			}
		}
	}


	if (UIHitTest(panel->position, panel->dimensions)
		&& (UIState.active_panel == NULL || UIState.active_panel == panel))
	{
		UIState.hot_panel = panel;

		if (UIState.active_panel == NULL && UIHitTest(panel->position, {panel->dimensions.x, UI_PANEL_TITLEBAR_HEIGHT})
			&& NewInput->mouse.left)
		{
			UIState.active_panel  = panel;
			panel->current_anchor = panel->position - NewInput->mouse.position;
			panel->depth		  = ++UIState.highest_depth_value;
		}

		else if (UIState.active_panel == panel && !NewInput->mouse.left)
		{
			UIState.active_panel = NULL;
		}
	}

	if (UIState.active_panel == panel)
	{
		UIState.hot_panel = panel;
		panel->position   = NewInput->mouse.position + panel->current_anchor;
	}
}
