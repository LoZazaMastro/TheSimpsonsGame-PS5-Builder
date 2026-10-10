/**
 * @file        rex/ui/overlay_input.h
 * @brief       Whether an overlay currently takes the keyboard and mouse.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */
#pragma once

namespace rex::ui {

// Overlays that take keyboard and mouse input themselves (settings, console)
// hold this while open. Keyboard & mouse controller emulation pauses and lets
// go of the mouse meanwhile, so the overlay can be clicked and typed in.
void AcquireOverlayInput();
void ReleaseOverlayInput();
bool IsOverlayInputActive();

}  // namespace rex::ui
