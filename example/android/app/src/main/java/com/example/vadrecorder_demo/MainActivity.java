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

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import androidx.core.app.ActivityCompat;

public class MainActivity extends Activity {
    private static final String TAG = "VadRecorderDemo";
    private long mVadRecorderHandle = 0;
    private AudioRecord mAudioRecord;
    Thread mRecordThread;
    private boolean mRecording = false;
    private boolean mStarted = false;
    private int mBufferSize;
    private TextView mStatusView;
    private static final String RECORD_VOICE_FILE = "record_voice.aac";
    private static final String  RECORD_TIMESTAMP_FILE = "record_timestamp.txt";
    private static String mRecordVoiceFile;
    private static String mRecordTimestampFile;
    private static final int PERMISSIONS_REQUEST_CODE_AUDIO = 1;
    private static final int PERMISSIONS_REQUEST_CODE_EXTERNAL_STORAGE = 2;

    public static void requestPermissions(Activity activity) {
        // request audio permissions
        if (ActivityCompat.checkSelfPermission(activity, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            String[] PERMISSION_AUDIO = { Manifest.permission.RECORD_AUDIO };
            ActivityCompat.requestPermissions(activity, PERMISSION_AUDIO, PERMISSIONS_REQUEST_CODE_AUDIO);
        }
        // request external storage permissions
        if (ActivityCompat.checkSelfPermission(activity, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED ||
            ActivityCompat.checkSelfPermission(activity, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            String[] PERMISSION_EXTERNAL_STORAGE =
                    new String[]{ Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE };
            ActivityCompat.requestPermissions(activity, PERMISSION_EXTERNAL_STORAGE, PERMISSIONS_REQUEST_CODE_EXTERNAL_STORAGE);
        }
    }

    @Override
    public void onRequestPermissionsResult(int permsRequestCode, String[] permissions, int[] grantResults) {
        switch (permsRequestCode) {
            case PERMISSIONS_REQUEST_CODE_AUDIO:
                if (grantResults[0] != PackageManager.PERMISSION_GRANTED){
                    Toast.makeText(this, "Failed request RECORD_AUDIO permission", Toast.LENGTH_LONG).show();
                }
                break;
            case PERMISSIONS_REQUEST_CODE_EXTERNAL_STORAGE:
                for (int result : grantResults) {
                    if (result != PackageManager.PERMISSION_GRANTED) {
                        Toast.makeText(this, "Failed request EXTERNAL_STORAGE permission", Toast.LENGTH_LONG).show();
                        break;
                    }
                }
                break;
            default:
                break;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        requestPermissions(this);

        mVadRecorderHandle = native_create();

        String sdcardPath = "/storage/emulated/0";
        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
            sdcardPath = Environment.getExternalStorageDirectory().getAbsolutePath();
        }
        mRecordVoiceFile = sdcardPath + "/" + RECORD_VOICE_FILE;
        mRecordTimestampFile = sdcardPath + "/" + RECORD_TIMESTAMP_FILE;

        mStatusView = findViewById(R.id.statusView);
        mStatusView.setText("Idle");
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy");
        if (mRecording) {
            stopRecording();
            try {
                mRecordThread.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        if (mAudioRecord != null) {
            mAudioRecord.stop();
            mAudioRecord.release();
        }
        native_destroy(mVadRecorderHandle);
        super.onDestroy();
    }

    public void onStartRecordingClick(View view) {
        if (!mStarted) {
            mStarted = true;
            startRecording();
            mStatusView.setText("Recording");
        }
    }

    public void onStopRecordingClick(View view) {
        if (mStarted) {
            stopRecording();
            mStarted = false;
            mStatusView.setText("Idle");
        }
    }

    private void startRecording() {
        Log.d(TAG, "startRecording");
        int sampleRate = 16000;
        int channelConfig = AudioFormat.CHANNEL_IN_MONO;
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        int bytesPer10Ms = sampleRate/100*2;
        mBufferSize = AudioRecord.getMinBufferSize(sampleRate, channelConfig, audioFormat);
        mBufferSize = (mBufferSize/bytesPer10Ms + 1)*bytesPer10Ms;
        mAudioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, sampleRate, channelConfig, audioFormat, mBufferSize);
        native_init(mVadRecorderHandle, mRecordVoiceFile, mRecordTimestampFile, sampleRate, 1, 16);

        mRecordThread = new Thread(new Runnable() {
            @Override
            public void run() {
                byte[] buffer = new byte[mBufferSize];
                mAudioRecord.startRecording();
                mRecording = true;
                while (mRecording) {
                    int read = mAudioRecord.read(buffer, 0, mBufferSize);
                    if (read > 0) {
                        native_feed(mVadRecorderHandle, buffer, read);
                    }
                }

                native_deinit(mVadRecorderHandle);
                mAudioRecord.stop();
                mAudioRecord.release();
                mAudioRecord = null;
                mRecording = false;
            }
        });

        mRecordThread.start();
    }

    public void stopRecording() {
        Log.d(TAG, "stopRecording");
        mRecording = false;
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    private native long native_create();
    private native boolean native_init(long handle, String voiceFile, String timestampFile,
                                       int sampleRate, int channels, int bitsPerSample) throws IllegalStateException;
    private native boolean  native_feed(long handle, byte[] buff, int size) throws IllegalStateException;
    private native void native_deinit(long handle) throws IllegalStateException;
    private native void native_destroy(long handle) throws IllegalStateException;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("vadrecorder-jni");
    }
}
