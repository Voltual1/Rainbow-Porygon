-keep class org.libsdl.app.** { *; }

-assumenosideeffects class android.util.Log { *; }
 
-assumenosideeffects class kotlinx.coroutines.DebugStrings {
    public static *** toString(...);
}

-keepclassmembers class org.apache.mina.transport.socket.nio.NioProcessor {
    protected <methods>;
    public <methods>;
}