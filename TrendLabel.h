#pragma once

#include <lvgl.h>
#include "TrendIndicator.h"

// Show trend on an LVGL label: hidden for NONE, "↑" / "↓" otherwise
inline void applyTrend(lv_obj_t* label, TrendIndicator::Trend trend) {
    switch (trend) {
        case TrendIndicator::Trend::UP:
            lv_label_set_text(label, "↑");
            lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
            break;
        case TrendIndicator::Trend::DOWN:
            lv_label_set_text(label, "↓");
            lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
            break;
        default:
            lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
            break;
    }
}
