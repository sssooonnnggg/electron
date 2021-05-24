// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/win_frame_view.h"

#include "base/win/windows_version.h"
#include "shell/browser/native_window_views.h"
#include "ui/views/widget/widget.h"
#include "ui/views/win/hwnd_util.h"

#include "chrome/browser/win/titlebar_config.h"
#include "shell/browser/ui/views/win_caption_button_container.h"
#include "ui/base/win/hwnd_metrics.h"
#include "ui/display/win/screen_win.h"

namespace electron {

const char WinFrameView::kViewClassName[] = "WinFrameView";

WinFrameView::WinFrameView() = default;
WinFrameView::~WinFrameView() = default;

void WinFrameView::Init(NativeWindowViews* window, views::Widget* frame) {
  window_ = window;
  frame_ = frame;
  caption_button_container_ =
      AddChildView(std::make_unique<WinCaptionButtonContainer>(this));
}

gfx::Rect WinFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  return views::GetWindowBoundsForClientBounds(
      static_cast<views::View*>(const_cast<WinFrameView*>(this)),
      client_bounds);
}

int WinFrameView::NonClientHitTest(const gfx::Point& point) {
  int button_hittest = caption_button_container_->NonClientHitTest(point);
  LOG(INFO) << "WinFrameView::NonClientHitTest - WinFrameView frame_view - "
            << static_cast<void*>(this) << " - " << __LINE__;
  if (button_hittest != HTCAPTION) {
    return button_hittest;
  }

  if (window_->has_frame()) {
    LOG(INFO) << "WinFrameView::NonClientHitTest - has_frame() returned TRUE - "
              << __LINE__;
    return frame_->client_view()->NonClientHitTest(point);
  } else {
    LOG(INFO)
        << "WinFrameView::NonClientHitTest - has_frame() returned FALSE - "
        << __LINE__;
    return FramelessView::NonClientHitTest(point);
  }
}

const char* WinFrameView::GetClassName() const {
  return kViewClassName;
}

bool WinFrameView::IsMaximized() const {
  return frame()->IsMaximized();
}

bool WinFrameView::ShouldCustomDrawSystemTitlebar() const {
  return window()->title_bar_style() !=
         NativeWindowViews::TitleBarStyle::kNormal;
}

void WinFrameView::Layout() {
  LayoutCaptionButtons();
  // if (browser_view()->IsWindowControlsOverlayEnabled())
  //   LayoutWindowControlsOverlay();
  // else
  //   LayoutTitleBar();
  // LayoutClientView();
  NonClientFrameView::Layout();
}

// SkColor WinFrameView::GetReadableFeatureColor(
//     SkColor background_color) {
//   // color_utils::GetColorWithMaxContrast()/IsDark() aren't used here because
//   // they switch based on the Chrome light/dark endpoints, while we want to
//   use
//   // the system native behavior below.
//   const auto windows_luma = [](SkColor c) {
//     return 0.25f * SkColorGetR(c) + 0.625f * SkColorGetG(c) +
//            0.125f * SkColorGetB(c);
//   };
//   return windows_luma(background_color) <= 128.0f ? SK_ColorWHITE
//                                                   : SK_ColorBLACK;

int WinFrameView::FrameTopBorderThickness(bool restored) const {
  // FIXME(@mlaurencin): I don't think either of these cases apply to us,
  // so I am just commenting it all out
  // const bool is_fullscreen =
  //     (frame()->IsFullscreen() || IsMaximized()) && !restored;

  // if (!is_fullscreen) {
  //   // Restored windows have a smaller top resize handle than the system
  //   // default. When maximized, the OS sizes the window such that the border
  //   // extends beyond the screen edges. In that case, we must return the
  //   default
  //   // value.
  //   if (browser_view()->GetTabStripVisible())
  //     return drag_handle_padding_;

  //   // There is no top border in tablet mode when the window is "restored"
  //   // because it is still tiled into either the left or right pane of the
  //   // display takes up the entire vertical extent of the screen. Note that a
  //   // rendering bug in Windows may still cause the very top of the window to
  //   be
  //   // cut off intermittently, but that's an OS issue that affects all
  //   // applications, not specifically Chrome.
  //   if (IsWebUITabStrip())
  //     return 0;
  // }

  // Mouse and touch locations are floored but GetSystemMetricsInDIP is rounded,
  // so we need to floor instead or else the difference will cause the hittest
  // to fail when it ought to succeed.
  return std::floor(
      FrameTopBorderThicknessPx(restored) /
      display::win::ScreenWin::GetScaleFactorForHWND(HWNDForView(this)));
}

int WinFrameView::FrameTopBorderThicknessPx(bool restored) const {
  // Distinct from FrameBorderThickness() because we can't inset the top
  // border, otherwise Windows will give us a standard titlebar.
  // For maximized windows this is not true, and the top border must be
  // inset in order to avoid overlapping the monitor above.

  // See comments in BrowserDesktopWindowTreeHostWin::GetClientAreaInsets().
  const bool needs_no_border =
      (ShouldCustomDrawSystemTitlebar() && frame()->IsMaximized()) ||
      frame()->IsFullscreen();
  if (needs_no_border && !restored)
    return 0;

  // Note that this method assumes an equal resize handle thickness on all
  // sides of the window.
  // TODO(dfried): Consider having it return a gfx::Insets object instead.
  return ui::GetFrameThickness(
      MonitorFromWindow(HWNDForView(this), MONITOR_DEFAULTTONEAREST));
}

int WinFrameView::TitlebarMaximizedVisualHeight() const {
  int maximized_height =
      display::win::ScreenWin::GetSystemMetricsInDIP(SM_CYCAPTION);
  // FIXME(@mlaurencin): I don't think we have this,
  // so I am just commenting this case all out
  // if (web_app_frame_toolbar()) {
  //   // Adding 2px of vertical padding puts at least 1 px of space on the top
  //   and
  //   // bottom of the element.
  //   constexpr int kVerticalPadding = 2;
  //   maximized_height = std::max(
  //       maximized_height,
  //       web_app_frame_toolbar()->GetPreferredSize().height() +
  //                             kVerticalPadding);
  // }
  return maximized_height;
}

int WinFrameView::TitlebarHeight(bool restored) const {
  if (frame()->IsFullscreen() && !restored)
    return 0;

  // The titlebar's actual height is the same in restored and maximized, but
  // some of it is above the screen in maximized mode. See the comment in
  // FrameTopBorderThicknessPx(). For WebUI, // FIXME(@mlaurencin): I don't
  // think this applies to us, so I am just returning the false case return
  // (IsWebUITabStrip()
  //             ? caption_button_container_->GetPreferredSize().height()
  //             : TitlebarMaximizedVisualHeight()) +
  //        FrameTopBorderThickness(false);

  return TitlebarMaximizedVisualHeight() + FrameTopBorderThickness(false);
}

int WinFrameView::WindowTopY() const {
  // The window top is SM_CYSIZEFRAME pixels when maximized (see the comment in
  // FrameTopBorderThickness()) and floor(system dsf) pixels when restored.
  // Unfortunately we can't represent either of those at hidpi without using
  // non-integral dips, so we return the closest reasonable values instead.
  if (IsMaximized())
    return FrameTopBorderThickness(false);
  // FIXME(@mlaurencin): I don't think this applies to us,
  // so I am just returning the false case
  // return IsWebUITabStrip() ? FrameTopBorderThickness(true) : 1;
  return 1;
}

void WinFrameView::LayoutCaptionButtons() {
  if (!caption_button_container_)
    return;

  // Non-custom system titlebar already contains caption buttons.
  if (!ShouldCustomDrawSystemTitlebar()) {
    LOG(INFO)
        << "WinFrameView::LayoutCaptionButtons - do NOT draw custom titlebar - "
        << __LINE__;
    caption_button_container_->SetVisible(false);
    return;
  }

  LOG(INFO) << "WinFrameView::LayoutCaptionButtons - DRAW custom titlebar - "
            << __LINE__;
  caption_button_container_->SetVisible(true);

  ///*
  const gfx::Size preferred_size =
      caption_button_container_->GetPreferredSize();
  int height = preferred_size.height();
  //*/

  // We use the standard caption bar height when maximized in tablet mode, which
  // is smaller than our preferred button size. <-- TODO(@mlaurencin): I believe
  // this casing is then not necessary for our purposes if (IsWebUITabStrip() &&
  // IsMaximized()) {
  //   height = std::min(height, TitlebarMaximizedVisualHeight());
  // } else if (browser_view()->IsWindowControlsOverlayEnabled()) { //
  // FIXME(@mlaurencin): Forcing it to be enabled for now
  //   // When the WCO is enabled, the caption button container should be the
  //   same
  //   // height as the WebAppFrameToolbar for a seamless overlay.
  //   height = IsMaximized() ? TitlebarMaximizedVisualHeight()
  //                          : TitlebarHeight(false) - WindowTopY();
  // }

  // FIXME(@mlaurencin): Remove after testing and just have the above remain

  ///*
  height = IsMaximized() ? TitlebarMaximizedVisualHeight()
                         : TitlebarHeight(false) - WindowTopY();

  caption_button_container_->SetBounds(width() - preferred_size.width(),
                                       WindowTopY(), preferred_size.width(),
                                       height);
  //*/
}

}  // namespace electron
