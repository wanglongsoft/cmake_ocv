package soft.wl.function;

public class FunctionControl {
    static {
        System.loadLibrary("function_control");
    }

    public FunctionControl() {
    }

    native public Object sendCommand(int cmd , Object in, Object out);
}
