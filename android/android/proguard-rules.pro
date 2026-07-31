-keep class org.libsdl.app.** { *; }

-assumenosideeffects class android.util.Log { *; }
 
-assumenosideeffects class kotlinx.coroutines.DebugStrings {
    public static *** toString(...);
}
