-keep class me.voltual.rb.** { *; }

-assumenosideeffects class android.util.Log { *; }
 
-assumenosideeffects class kotlinx.coroutines.DebugStrings {
    public static *** toString(...);
}
