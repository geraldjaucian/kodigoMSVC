package app.kodigo.gj.pixelized;

import android.app.Activity;
import android.graphics.Point;
import android.graphics.Rect;
import android.nfc.Tag;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.Surface;
import android.view.Window;
import android.view.WindowManager;

public class MainActivity extends Activity implements SurfaceHolder.Callback2 {

    public static final String TAG = "pixelized_MainActivity";
    private static Handler handler = new Handler();

    MainView mView;
    Point screenSize = new Point();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Window window = getWindow();
        View decor = window.getDecorView();
        int flags = WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            flags |= WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS;
        }
        window.addFlags(flags);
        window.takeSurface(this);
        decor.setFitsSystemWindows(true);

//        window.takeInputQueue(this);
        mView = new MainView(this);
        setContentView(mView);
        mView.requestFocus();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            getWindow().getWindowManager().getDefaultDisplay().getRealSize(screenSize);
        } else {
            getWindow().getWindowManager().getDefaultDisplay().getSize(screenSize);
        }

//        Log.d(TAG, "onCreate: surface: " + screenSize.toString());
		nativeOnCreate();
    }

    @Override
    protected void onResume() {
        super.onResume();
//        Log.d(TAG, "onResume: ");
        int flags = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            flags |= View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
            flags |= View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            flags |= View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
        }
        getWindow().getDecorView().setSystemUiVisibility(flags);
        nativeOnResume();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
//        Log.d(TAG, "onWindowFocusChanged: " + hasFocus);
        nativeOnFocusChanged(hasFocus);
    }

    @Override
    protected void onPause() {
        super.onPause();
//        Log.d(TAG, "onPause: ");
        nativeOnPause();
    }

	@Override
    protected void onDestroy() {
        super.onDestroy();
//        Log.d(TAG, "onDestroy: ");
        nativeOnDestroy();
    }

    @Override
    public void surfaceRedrawNeeded(SurfaceHolder holder) {
    }


    @Override
    public void surfaceCreated(SurfaceHolder holder) {
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        nativeSurfaceChanged(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        nativeSurfaceDestroyed(holder.getSurface());
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        nativeOnTouchEvent(event.getAction(), event.getActionIndex(), event.getX(), event.getY(), event.getRawX(), event.getRawY());
        return false;
    }

    
    static class MainView extends View {
        MainActivity mActivity;
        Rect viewRect = new Rect();
        Rect usableRect = new Rect();

        Runnable onLayoutRunnable = new Runnable() {
            @Override
            public void run() {
                mActivity.nativeOnLayout(viewRect.left, viewRect.right, viewRect.top, viewRect.bottom,
                        usableRect.left, usableRect.right, usableRect.top, usableRect.bottom);
            }
        };

        public MainView(MainActivity activity) {
            super(activity);
            mActivity = activity;
        }

        @Override
        protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
            super.onLayout(changed, left, top, right, bottom);
            handler.removeCallbacksAndMessages(null);
            getWindowVisibleDisplayFrame(usableRect);
            viewRect.set(left, top, right, bottom);
            handler.postDelayed(onLayoutRunnable, 500);
        }
    }

    static {
    	System.loadLibrary("Pixelized");
    }

	private native void nativeOnCreate();
    private native void nativeOnResume();
    private native void nativeOnFocusChanged(boolean focus);
    private native void nativeOnPause();
    private native void nativeOnDestroy();

    private native void nativeSurfaceChanged(Surface surface);
    private native void nativeSurfaceDestroyed(Surface surface);
    private native void nativeOnTouchEvent(int state, int index, float x, float y, float rawX, float rawY);
    public native void nativeOnLayout(int viewLeft, int viewRight, int viewTop, int viewBottom, int usableLeft, int usableRight, int usableTop, int usableBottom);
}