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
package me.voltual.rp;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import java.io.IOException;
import java.util.Arrays;

import org.libsdl.app.SDLActivity;

public final class GbaControlsView extends View {
    private static final int A = 1 << 0;
    private static final int B = 1 << 1;
    private static final int SELECT = 1 << 2;
    private static final int START = 1 << 3;
    private static final int RIGHT = 1 << 4;
    private static final int LEFT = 1 << 5;
    private static final int UP = 1 << 6;
    private static final int DOWN = 1 << 7;
    private static final int R = 1 << 8;
    private static final int L = 1 << 9;

    private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint outline = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint text = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint borderPaint = new Paint(Paint.FILTER_BITMAP_FLAG);
    private final Bitmap[] backgrounds = new Bitmap[15];
    private int backgroundCount;
    private Bitmap border;
    private int pressed;

    public GbaControlsView(Context context) {
        super(context);
        setBackgroundColor(Color.TRANSPARENT);
        outline.setColor(Color.argb(170, 255, 255, 255));
        outline.setStyle(Paint.Style.STROKE);
        outline.setStrokeWidth(3);
        text.setColor(Color.WHITE);
        text.setTextAlign(Paint.Align.CENTER);
        text.setFakeBoldText(true);
        try {
            backgrounds[0] = BitmapFactory.decodeStream(context.getAssets().open("BG.png"));
            backgroundCount = 1;
            for (int i = 1; i < backgrounds.length; i++) {
                try {
                    backgrounds[i] = BitmapFactory.decodeStream(context.getAssets().open("BG" + i + ".png"));
                    backgroundCount++;
                } catch (IOException ignored) {
                    break;
                }
            }
            border = BitmapFactory.decodeStream(context.getAssets().open("Border.png"));
        } catch (IOException ignored) {
            border = null;
        }
    }

    private int sideWidth() {
        return Math.max((getWidth() - getHeight() * 3 / 2) / 2, getWidth() * 14 / 100);
    }

    private int unit() {
        return Math.min(sideWidth() / 3, getHeight() / 8);
    }

    private RectF controlRect(int control) {
        int side = sideWidth();
        int pad = unit();
        int padX = side * 2 / 3;
        int padY = getHeight() * 7 / 10;
        int button = Math.min(side * 2 / 5, getHeight() / 6);

        switch (control) {
        case UP:     return new RectF(padX - pad / 2f, padY - pad * 1.5f, padX + pad / 2f, padY - pad / 2f);
        case DOWN:   return new RectF(padX - pad / 2f, padY + pad / 2f, padX + pad / 2f, padY + pad * 1.5f);
        case LEFT:   return new RectF(padX - pad * 1.5f, padY - pad / 2f, padX - pad / 2f, padY + pad / 2f);
        case RIGHT:  return new RectF(padX + pad / 2f, padY - pad / 2f, padX + pad * 1.5f, padY + pad / 2f);
        case A:      return new RectF(getWidth() - side / 4f - button, getHeight() * .58f,
                                      getWidth() - side / 4f, getHeight() * .58f + button);
        case B:      return new RectF(getWidth() - side + side / 4f, getHeight() * .76f,
                                      getWidth() - side + side / 4f + button, getHeight() * .76f + button);
        case SELECT: return new RectF(side / 10f, getHeight() * .25f, side * .9f, getHeight() * .35f);
        case START:  return new RectF(getWidth() - side * .9f, getHeight() * .25f,
                                      getWidth() - side * .1f, getHeight() * .35f);
        case L:      return new RectF(side / 10f, getHeight() * .05f, side * .9f, getHeight() * .15f);
        case R:      return new RectF(getWidth() - side * .9f, getHeight() * .05f,
                                      getWidth() - side * .1f, getHeight() * .15f);
        default:     return new RectF();
        }
    }

    private int controlsAt(float x, float y) {
        int result = 0;
        int[] controls = {A, B, SELECT, START, RIGHT, LEFT, UP, DOWN, R, L};
        for (int control : controls) {
            if (controlRect(control).contains(x, y)) {
                result |= control;
            }
        }
        return result;
    }

    private int keyCode(int control) {
        switch (control) {
        case A:      return KeyEvent.KEYCODE_Z;
        case B:      return KeyEvent.KEYCODE_X;
        case SELECT: return KeyEvent.KEYCODE_BACKSLASH;
        case START:  return KeyEvent.KEYCODE_ENTER;
        case RIGHT:  return KeyEvent.KEYCODE_DPAD_RIGHT;
        case LEFT:   return KeyEvent.KEYCODE_DPAD_LEFT;
        case UP:     return KeyEvent.KEYCODE_DPAD_UP;
        case DOWN:   return KeyEvent.KEYCODE_DPAD_DOWN;
        case R:      return KeyEvent.KEYCODE_S;
        case L:      return KeyEvent.KEYCODE_A;
        default:     return KeyEvent.KEYCODE_UNKNOWN;
        }
    }

    private void setPressed(int next) {
        int changed = pressed ^ next;
        int[] controls = {A, B, SELECT, START, RIGHT, LEFT, UP, DOWN, R, L};
        for (int control : controls) {
            if ((changed & control) != 0) {
                int action = (next & control) != 0 ? KeyEvent.ACTION_DOWN : KeyEvent.ACTION_UP;
                KeyEvent event = new KeyEvent(action, keyCode(control));
                SDLActivity.handleKeyEvent(this, event.getKeyCode(), event, null);
            }
        }
        pressed = next;
        invalidate();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_CANCEL) {
            setPressed(0);
            return true;
        }

        int releasedPointer = event.getActionMasked() == MotionEvent.ACTION_UP
| event.getActionMasked() == MotionEvent.ACTION_POINTER_UP
                ? event.getActionIndex() : -1;
        int next = 0;
        for (int i = 0; i < event.getPointerCount(); i++) {
            if (i != releasedPointer) {
                next |= controlsAt(event.getX(i), event.getY(i));
            }
        }
        setPressed(next);
        return true;
    }

    private void drawControl(Canvas canvas, int control, String label) {
        RectF rect = controlRect(control);
        fill.setColor(Color.argb((pressed & control) != 0 ? 155 : 70, 255, 255, 255));
        canvas.drawRect(rect, fill);
        canvas.drawRect(rect, outline);
        if (label != null) {
            text.setTextSize(Math.min(rect.height() * .55f, rect.width() / Math.max(label.length() * .6f, 1)));
            Paint.FontMetrics metrics = text.getFontMetrics();
            float baseline = rect.centerY() - (metrics.ascent + metrics.descent) / 2;
            canvas.drawText(label, rect.centerX(), baseline, text);
        }
    }

    private void drawBorder(Canvas canvas) {
        int scaleSetting = getPlatformSetting(2); // PLATFORM_SETTING_INTEGER_SCALE
        int gameWidth, gameHeight;
        
        if (scaleSetting == 1) {
            int scale = Math.max(1, Math.min(getWidth() / 240, getHeight() / 160));
            gameWidth = 240 * scale;
            gameHeight = 160 * scale;
        } else if (scaleSetting == 2) {
            gameWidth = getWidth();
            gameHeight = getHeight();
        } else {
            float scaleX = (float) getWidth() / 240f;
            float scaleY = (float) getHeight() / 160f;
            if (scaleX < scaleY) {
                gameWidth = getWidth();
                gameHeight = Math.round(getWidth() * 160f / 240f);
            } else {
                gameHeight = getHeight();
                gameWidth = Math.round(getHeight() * 240f / 160f);
            }
        }

        int gameX = (getWidth() - gameWidth) / 2;
        int gameY = (getHeight() - gameHeight) / 2;

        if (scaleSetting != 2) {
            int backgroundOption = getBorderBackground();
            if (backgroundOption < backgroundCount && backgrounds[backgroundOption] != null) {
                Bitmap background = backgrounds[backgroundOption];
                Rect output = new Rect(0, 0, getWidth(), getHeight());
                Rect[] regions = {
                        new Rect(0, 0, getWidth(), gameY),
                        new Rect(0, gameY + gameHeight, getWidth(), getHeight()),
                        new Rect(0, gameY, gameX, gameY + gameHeight),
                        new Rect(gameX + gameWidth, gameY, getWidth(), gameY + gameHeight)
                };
                for (Rect region : regions) {
                    int state = canvas.save();
                    canvas.clipRect(region);
                    canvas.drawBitmap(background, null, output, borderPaint);
                    canvas.restoreToCount(state);
                }
            }
        }

        if (getPlatformSetting(4) != 0 && border != null) {
            int innerWidth = gameWidth - 2;
            int innerHeight = gameHeight - 2;
            canvas.drawBitmap(border, new Rect(141, 18, 1141, 701),
                    new Rect(
                            gameX + 1 - innerWidth * 19 / 961,
                            gameY + 1 - innerHeight * 20 / 643,
                            gameX + 1 + innerWidth + innerWidth * 20 / 961,
                            gameY + 1 + innerHeight + innerHeight * 20 / 643),
                    borderPaint);
        }
    }

    private static native int getBorderBackground();
    private static native int getPlatformSetting(int setting);

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        drawBorder(canvas);
        drawControl(canvas, UP, null);
        drawControl(canvas, DOWN, null);
        drawControl(canvas, LEFT, null);
        drawControl(canvas, RIGHT, null);
        drawControl(canvas, A, "A");
        drawControl(canvas, B, "B");
        drawControl(canvas, SELECT, "SELECT");
        drawControl(canvas, START, "START");
        drawControl(canvas, L, "L");
        drawControl(canvas, R, "R");
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.Q) {
            setSystemGestureExclusionRects(Arrays.asList(
                    new Rect(0, height / 2, width / 5, height),
                    new Rect(width * 4 / 5, height / 2, width, height)));
        }
    }
}