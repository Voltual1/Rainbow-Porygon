// MIT License
// 
// Copyright (c) 2026 pokeemerald-multiplatform contributors
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of the original multiplatform-port modifications contributed through this
// fork (the "Port Modifications"), to deal in the Port Modifications without
// restriction, including without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell copies of the Port
// Modifications, and to permit persons to whom the Port Modifications are
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Port Modifications.
// 
// THE PORT MODIFICATIONS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
// EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE PORT MODIFICATIONS OR THE USE OR OTHER
// DEALINGS IN THE PORT MODIFICATIONS.
// 
// Scope
// -----
// 
// This license applies only to original multiplatform-port modifications made by
// contributors to this fork. It does not grant rights to, or relicense:
// 
// - The upstream pokeemerald decompilation or contributions from its authors.
// - Pokemon Emerald, Pokemon characters, names, graphics, audio, story, or other
//   copyrighted or trademarked material owned by Nintendo, Creatures Inc., GAME
//   FREAK inc., or other respective owners.
// - Third-party software included in this repository, which remains subject to
//   its own license terms.
// 
// Users are responsible for determining which portions of a distribution are
// covered by this license and for complying with all applicable third-party and
// upstream terms.
package me.voltual.rp.legacy;

import android.graphics.Rect;
import android.os.Bundle;
import android.os.Build;
import me.voltual.rp.GbaControlsView;
import android.view.View;
import android.view.ViewGroup;

import java.util.Arrays;

import org.libsdl.app.SDLActivity;

public class PokeEmeraldActivity extends SDLActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        GbaControlsView controls = new GbaControlsView(this);
        mLayout.addView(controls, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
    }

    @Override
    public void setOrientationBis(int width, int height, boolean resizable, String hint) {
        // The manifest already keeps this activity in sensor landscape mode.
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (!hasFocus) {
            return;
        }

        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q && mSurface != null) {
            mSurface.post(() -> {
                int width = mSurface.getWidth();
                int height = mSurface.getHeight();
                mSurface.setSystemGestureExclusionRects(Arrays.asList(
                        new Rect(0, height / 2, width / 5, height),
                        new Rect(width * 4 / 5, height / 2, width, height)));
            });
        }
    }

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }
}
