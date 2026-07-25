package com.pokeemerald.experimental;

import android.graphics.Rect;
import android.os.Bundle;
import android.os.Build;
import android.view.View;
import android.view.ViewGroup;
import android.media.MediaPlayer;
import android.media.AudioTrack;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.content.res.AssetFileDescriptor;
import android.util.Log;

import java.util.Arrays;
import java.io.IOException;

import org.libsdl.app.SDLActivity;

public class PokeEmeraldActivity extends SDLActivity {
    private static final String TAG = "PokeEmeraldActivity";
    private MediaPlayer mMediaPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        GbaControlsView controls = new GbaControlsView(this);
        mLayout.addView(controls, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));

        // 启动独立线程进行音频测试，避免阻塞主 UI 线程
        testAudioPlayback();
    }

    private void testAudioPlayback() {
        new Thread(() -> {
            try {
                Log.d(TAG, "CAN DEBUG: Attempting to play mus_littleroot_test.mid from assets...");
                mMediaPlayer = new MediaPlayer();
                AssetFileDescriptor afd = getAssets().openFd("mus_littleroot_test.mid");
                mMediaPlayer.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
                afd.close();
                mMediaPlayer.prepare();
                mMediaPlayer.start();
                Log.d(TAG, "CAN DEBUG: mus_littleroot_test.mid started playing successfully.");
            } catch (IOException e) {
                Log.e(TAG, "CAN DEBUG: Failed to play MIDI from assets (file might not be in assets/): " + e.getMessage());
                Log.d(TAG, "CAN DEBUG: Falling back to generated AudioTrack beep test...");
                playTestBeep();
            }
        }).start();
    }

    private void playTestBeep() {
        int sampleRate = 44100;
        int numSamples = sampleRate * 2; // 持续 2 秒
        double[] sample = new double[numSamples];
        byte[] generatedSnd = new byte[2 * numSamples];

        // 填充 440Hz 经典 A4 音高正弦波
        for (int i = 0; i < numSamples; ++i) {
            sample[i] = Math.sin(2 * Math.PI * i / (sampleRate / 440.0));
        }

        // 转化为 16-bit 线性 PCM 音频数据
        int idx = 0;
        for (double dVal : sample) {
            short val = (short) (dVal * 32767);
            generatedSnd[idx++] = (byte) (val & 0x00ff);
            generatedSnd[idx++] = (byte) ((val & 0xff00) >>> 8);
        }

        try {
            AudioTrack audioTrack = new AudioTrack(
                    AudioManager.STREAM_MUSIC,
                    sampleRate,
                    AudioFormat.CHANNEL_OUT_MONO,
                    AudioFormat.ENCODING_PCM_16BIT,
                    generatedSnd.length,
                    AudioTrack.MODE_STATIC);
            
            audioTrack.write(generatedSnd, 0, generatedSnd.length);
            audioTrack.play();
            Log.d(TAG, "CAN DEBUG: AudioTrack test beep played successfully.");
        } catch (Exception e) {
            Log.e(TAG, "CAN DEBUG: AudioTrack failed to play: " + e.getMessage());
        }
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
    protected void onDestroy() {
        super.onDestroy();
        if (mMediaPlayer != null) {
            mMediaPlayer.release();
            mMediaPlayer = null;
        }
    }

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }
}