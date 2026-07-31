-keepnames class ** { *; }

-assumenosideeffects class android.util.Log { *; }
 
-assumenosideeffects class kotlinx.coroutines.DebugStrings {
    public static *** toString(...);
}
