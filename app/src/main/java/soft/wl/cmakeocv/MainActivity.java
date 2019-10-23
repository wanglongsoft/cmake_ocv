package soft.wl.cmakeocv;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;

import java.io.IOException;
import java.io.InputStream;

import soft.wl.function.FunctionControl;

public class MainActivity extends AppCompatActivity {

    private final static String TAG = "FunctionControl";

    private FunctionControl mFunctionControl = null;

    private Bitmap mBitmap = null;
    private ImageView mImageView = null;
    private Button mButton = null;
    private Object mOut = null;
    private Bitmap mOutBitmap = null;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mImageView = (ImageView) findViewById(R.id.image_view);
        mButton = (Button) findViewById(R.id.button_view);

        mFunctionControl = new FunctionControl();

        AsyncTask asyncTask = new AsyncTask() {
            @Override
            protected Object doInBackground(Object[] objects) {
                InputStream stream = null;
                try {
                    stream = getAssets().open("shenfen.jpg");
                    mBitmap = BitmapFactory.decodeStream(stream);
                } catch (IOException e) {
                    e.printStackTrace();
                } finally {
                    if (stream != null) {
                        try {
                            stream.close();
                        } catch (IOException e) {
                            // do nothing here
                        }
                    }
                }
                return null;
            }

            @Override
            protected void onPostExecute(Object o) {
                mImageView.setImageBitmap(mBitmap);
            }
        };
        asyncTask.execute();

        mButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.d(TAG, "onClick: ");
                mOut = mFunctionControl.sendCommand(1024, mBitmap, Bitmap.Config.ARGB_8888);
                if (mOut instanceof Bitmap) {
                    Log.d(TAG, "onClick: mOut is Bitmap");
                    mOutBitmap = (Bitmap) mOut;
                    mImageView.setImageBitmap(mOutBitmap);
                } else {
                    Log.d(TAG, "onClick mOutBitMap is not BitMap");
                }
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy: ");
        if(null != mBitmap) {
            mBitmap.recycle();
        }
        if(null != mOutBitmap) {
            mOutBitmap.recycle();
        }
    }

}
