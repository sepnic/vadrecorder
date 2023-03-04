/*
 * Copyright (C) 2023- Qinglong<sysu.zqlong@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.vadrecorder_demo;

import android.app.Service;
import android.content.Intent;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Handler;
import android.os.IBinder;
import android.util.Base64;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

public class SilenceMusicService extends Service implements MediaPlayer.OnCompletionListener {
    private MediaPlayer mMediaPlayer = null;
    private AudioManager mAudioManager = null;
    private final Handler mHandler = new Handler();
    private boolean mStopped = false;
    private File mSilenceMp3File;
    private static final String mSilenceMp3base64 =
        "//OIxAAAAAAAAAAAAFhpbmcAAAAPAAAAHgAABVgACAgIERERGRkZIiIiIioqKjMzMzs" +
        "7OztERERMTExVVVVVXV1dZmZmbm5ubnd3d4CAgIiIiIiRkZGZmZmioqKiqqqqs7Ozu7" +
        "u7u8TExMzMzNXV1dXd3d3m5ubu7u7u9/f3////AAAAUExBTUUzLjEwMARQAAAAAAAAA" +
        "AAVCCQCwCEAAeAAAAVY6TBM5AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" +
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" +
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA//MYxAAAAANIAAAAAE" +
        "xBTUUzLjEwMFVVVVVVVVVVVVVVVUxB//MYxBcAAANIAAAAAE1FMy4xMDBVVVVVVVVVV" +
        "VVVVVVVVUxB//MYxC4AAANIAAAAAE1FMy4xMDBVVVVVVVVVVVVVVVVVVUxB//MYxEUA" +
        "AANIAAAAAE1FMy4xMDBVVVVVVVVVVVVVVVVVVUxB//MYxFwAAANIAAAAAE1FMy4xMDB" +
        "VVVVVVVVVVVVVVVVVVUxB//MYxHMAAANIAAAAAE1FMy4xMDBVVVVVVVVVVVVVVVVVVU" +
        "xB//MYxIoAAANIAAAAAE1FMy4xMDBVVVVVVVVVVVVVVVVVVUxB//MYxKEAAANIAAAAA" +
        "E1FMy4xMDBVVVVVVVVVVVVVVVVVVUxB//MYxLgAAANIAAAAAE1FMy4xMDBVVVVVVVVV" +
        "VVVVVVVVVUxB//MYxM8AAANIAAAAAE1FMy4xMDBVVVVVVVVVVVVVVVVVVUxB//MYxOY" +
        "AAANIAAAAAE1FMy4xMDBVVVVVVVVVVVVVVVVVVUxB//MYxOgAAANIAAAAAE1FMy4xMD" +
        "BVVVVVVVVVVVVVVVVVVUxB//MYxOgAAANIAAAAAE1FMy4xMDBVVVVVVVVVVVVVVVVVV" +
        "UxB//MYxOgAAANIAAAAAE1FMy4xMDBVVVVVVVVVVVVVVVVVVUxB//MYxOgAAANIAAAA" +
        "AE1FMy4xMDBVVVVVVVVVVVVVVVVVVUxB//MYxOgAAANIAAAAAE1FMy4xMDBVVVVVVVV" +
        "VVVVVVVVVVUxB//MYxOgAAANIAAAAAE1FMy4xMDBVVVVVVVVVVVVVVVVVVUxB//MYxO" +
        "gAAANIAAAAAE1FMy4xMDBVVVVVVVVVVVVVVVVVVUxB//MYxOgAAANIAAAAAE1FMy4xM" +
        "DBVVVVVVVVVVVVVVVVVVUxB//MYxOgAAANIAAAAAE1FMy4xMDBVVVVVVVVVVVVVVVVV" +
        "VVVV//MYxOgAAANIAAAAAFVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV//MYxOgAAANIAAA" +
        "AAFVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV//MYxOgAAANIAAAAAFVVVVVVVVVVVVVVVV" +
        "VVVVVVVVVVVVVV//MYxOgAAANIAAAAAFVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV//MYx" +
        "OgAAANIAAAAAFVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV//MYxOgAAANIAAAAAFVVVVVV" +
        "VVVVVVVVVVVVVVVVVVVVVVVV//MYxOgAAANIAAAAAFVVVVVVVVVVVVVVVVVVVVVVVVV" +
        "VVVVV//MYxOgAAANIAAAAAFVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV//MYxOgAAANIAA" +
        "AAAFVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV//MYxOgAAANIAAAAAFVVVVVVVVVVVVVVV" +
        "VVVVVVVVVVVVVVV";

    private final AudioManager.OnAudioFocusChangeListener mAudioFocusChange = new
            AudioManager.OnAudioFocusChangeListener() {
                @Override
                public void onAudioFocusChange(int focusChange) {
                    switch (focusChange) {
                        case AudioManager.AUDIOFOCUS_GAIN:
                            startMediaPlayer();
                            break;
                        case AudioManager.AUDIOFOCUS_LOSS:
                            mAudioManager.abandonAudioFocus(mAudioFocusChange);
                            break;
                        case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                            break;
                        case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                            break;
                    }
                }
            };

    public SilenceMusicService() {
        try {
            byte[] silenceMp3ByteArray = Base64.decode(mSilenceMp3base64, Base64.DEFAULT);
            mSilenceMp3File = File.createTempFile("silence", ".mp3");
            mSilenceMp3File.deleteOnExit();
            FileOutputStream fos = new FileOutputStream(mSilenceMp3File);
            fos.write(silenceMp3ByteArray);
            fos.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO: Return the communication channel to the service.
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mStopped = false;
    }

    @Override
    public void onDestroy() {
        mStopped = true;
        stopMediaPlayer();
        super.onDestroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        mAudioManager = (AudioManager) getSystemService(AUDIO_SERVICE);
        if (mAudioManager != null)
            mAudioManager.requestAudioFocus(mAudioFocusChange, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
        if (mMediaPlayer == null) {
            mMediaPlayer = MediaPlayer.create(getApplicationContext(), Uri.parse(mSilenceMp3File.getAbsolutePath()));
            mMediaPlayer.setOnCompletionListener(this);
        }
        startMediaPlayer();
        return START_STICKY;
    }

    @Override
    public void onCompletion(MediaPlayer mp) {
        if (!mStopped) {
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    startMediaPlayer();
                }
            }, 10 * 1000);
        }
    }

    private void startMediaPlayer() {
        if (mMediaPlayer != null && !mMediaPlayer.isPlaying()) {
            mMediaPlayer.start();
        }
    }

    private void stopMediaPlayer() {
        if (mMediaPlayer != null) {
            mMediaPlayer.stop();
            mMediaPlayer.release();
            mMediaPlayer = null;
        }
    }
}
