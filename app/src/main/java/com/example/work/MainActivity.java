package com.example.work;

import android.Manifest;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.blankj.utilcode.util.UriUtils;
import com.example.work.select.FileUtils;
import com.example.work.select.ImageUtils;
import com.example.work.select.PermissionUtils;
import com.example.work.select.PictureBean;
import com.example.work.select.PictureSelector;
import com.yalantis.ucrop.UCrop;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private ImageView mOriginImageView, mResultImageView, mResultImageView2;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mOriginImageView = findViewById(R.id.origin_image);
        mResultImageView = findViewById(R.id.result_image);
        mResultImageView2 = findViewById(R.id.result_image2);

        findViewById(R.id.select_image).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                PictureSelector.create(MainActivity.this, PictureSelector.SELECT_REQUEST_CODE)
                        .selectPicture(true);
            }
        });

    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        /*结果回调*/
        if (resultCode == RESULT_OK && requestCode == PictureSelector.SELECT_REQUEST_CODE) {
            if (data != null) {
                PictureBean pictureBean = data.getParcelableExtra(PictureSelector.PICTURE_RESULT);

                Log.d(TAG, "onActivityResult: 1 pictureBean==null?" + (pictureBean == null) + "\nis cut?" + pictureBean.isCut());
                String path = pictureBean.isCut() ? pictureBean.getPath() : UriUtils.uri2File(pictureBean.getUri()).getAbsolutePath();
                Log.d(TAG, "onActivityResult: 2 path=" + path);
                Bitmap originBitmap = BitmapFactory.decodeFile(path);
                Log.d(TAG, "onActivityResult: 3 originBitmap=" + originBitmap + " width=" + originBitmap.getWidth() + " height=" + originBitmap.getHeight());
                mOriginImageView.setImageBitmap(originBitmap);
                Log.d(TAG, "onActivityResult: 4 ok");

                Bitmap[] result = clipImage(path, 0, 0, originBitmap.getWidth(), originBitmap.getHeight());
                if (result.length == 1) {
                    Toast.makeText(this, "选择的图片没有检测到直线！重新选择！", Toast.LENGTH_SHORT).show();
                    return;
                }
                Log.d(TAG, "onActivityResult: 5 length:" + result.length);
                mResultImageView.setImageBitmap(result[0]);
                Log.d(TAG, "onActivityResult: 6 end");
                mResultImageView2.setImageBitmap(result[1]);
            }
        } else if (resultCode == RESULT_OK && requestCode == UCrop.REQUEST_CROP) {
            final Uri resultUri = UCrop.getOutput(data);
            String path = ImageUtils.getImagePath(this, resultUri);
            Log.d(TAG, "onActivityResult: 2 path=" + path);
            Bitmap originBitmap = BitmapFactory.decodeFile(path);
            Log.d(TAG, "onActivityResult: 3 originBitmap=" + originBitmap + " width=" + originBitmap.getWidth() + " height=" + originBitmap.getHeight());
            mOriginImageView.setImageBitmap(originBitmap);
            Log.d(TAG, "onActivityResult: 4 ok");

            Bitmap[] result = clipImage(path, 0, 0, originBitmap.getWidth(), originBitmap.getHeight());
            if (result.length == 1) {
                Toast.makeText(this, "选择的图片没有检测到直线！重新选择！", Toast.LENGTH_SHORT).show();
                return;
            }
            Log.d(TAG, "onActivityResult: 5 length:" + result.length);
            mResultImageView.setImageBitmap(result[0]);
            Log.d(TAG, "onActivityResult: 6 end");
            mResultImageView2.setImageBitmap(result[1]);
        } else if (resultCode == UCrop.RESULT_ERROR) {
            final Throwable cropError = UCrop.getError(data);
            if (cropError != null) {
                Toast.makeText(this, "error: " + cropError.toString(), Toast.LENGTH_SHORT).show();
            }
        } else {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    public native Bitmap[] clipImage(String path, int left, int top, int right, int bottom);

}
