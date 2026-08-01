//Copyright (C) 2025 Voltual
// 本程序是自由软件：你可以根据自由软件基金会发布的 GNU 通用公共许可证第3版
//（或任意更新的版本）的条款重新分发和/或修改它。
//本程序是基于希望它有用而分发的，但没有任何担保；甚至没有适销性或特定用途适用性的隐含担保。
// 有关更多细节，请参阅 GNU 通用公共许可证。
//
// 你应该已经收到了一份 GNU 通用公共许可证的副本
// 如果没有，请查阅 <http://www.gnu.org/licenses/>.
package me.voltual.rp.ui

import android.view.ViewGroup
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.viewinterop.AndroidView
import androidx.lifecycle.viewmodel.navigation3.rememberViewModelStoreNavEntryDecorator
import androidx.navigation3.runtime.*
import androidx.navigation3.scene.DialogSceneStrategy
import androidx.navigation3.ui.NavDisplay
import me.voltual.rp.ui.settings.ftp.FtpSettingsScreen
import me.voltual.rp.core.ui.animation.*
import me.voltual.rp.core.ui.theme.ThemeCustomizeScreen
import me.voltual.rp.ui.settings.update.UpdateSettingsScreen
import me.voltual.rp.ui.settings.update.UpdateSettingsViewModel
import org.koin.compose.viewmodel.koinViewModel
import org.libsdl.app.SDLActivity

@Composable
fun BBQNavDisplay(
    backStack: List<NavKey>,
    onBack: () -> Unit,
    snackbarHostState: SnackbarHostState,
    modifier: Modifier = Modifier,
    platformEntryProvider: @Composable (NavKey) -> (@Composable () -> Unit)? = { null }
) {
    val mySceneStrategy = remember { DialogSceneStrategy<NavKey>() }
    val slideDistance = rememberSlideDistance()

    val decorators = listOf(
        rememberSaveableStateHolderNavEntryDecorator<NavKey>(),
        rememberViewModelStoreNavEntryDecorator<NavKey>()
    )

    NavDisplay(
        backStack = backStack,
        onBack = onBack,
        entryDecorators = decorators,
        modifier = modifier.fillMaxSize(),
        sceneStrategy = mySceneStrategy,

        transitionSpec = {
            materialSharedAxisX(
                forward = true,
                slideDistance = slideDistance
            )
        },

        popTransitionSpec = {
            materialSharedAxisX(
                forward = false,
                slideDistance = slideDistance
            )
        },

        entryProvider = { key ->
            NavEntry(key) {
                val platformContent = platformEntryProvider(key)
                if (platformContent != null) {
                    platformContent()
                } else {
                    when (key) {
                        is Home -> {
                            Box(modifier = Modifier.fillMaxSize()) {
                                AndroidView(
                                    factory = { context ->
                                        val layout = SDLActivity.getLayout()
                                        if (layout.parent != null) {
                                            (layout.parent as ViewGroup).removeView(layout)
                                        }
                                        layout
                                    },
                                    modifier = Modifier.fillMaxSize()
                                )
                            }
                        }

                        is ThemeCustomize -> {
                            ThemeCustomizeScreen(modifier = Modifier.fillMaxSize())
                        }

                        is UpdateSettings -> {
                            val viewModel: UpdateSettingsViewModel = koinViewModel()
                            UpdateSettingsScreen(
                                viewModel = viewModel,
                                snackbarHostState = snackbarHostState
                            )
                        }
                        
                        is FtpSettings -> {
                            FtpSettingsScreen(
                                modifier = Modifier.fillMaxSize(),
                                snackbarHostState = snackbarHostState
                            )
                        }

                        else -> {
                            Box(
                                modifier = Modifier.fillMaxSize(),
                                contentAlignment = Alignment.Center,
                            ) {
                                Text("Unknown Key: ${key::class.simpleName}", color = Color.Red)
                            }
                        }
                    }
                }
            }
        }
    )
}