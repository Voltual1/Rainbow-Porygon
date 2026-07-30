// Copyright (C) 2025 Voltual
// 本程序是自由软件：你可以根据自由软件基金会发布的 GNU 通用公共许可证第3版

package me.voltual.rp

import androidx.compose.foundation.*
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.focus.FocusManager
import androidx.compose.ui.unit.dp

import androidx.compose.material3.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.History
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material.icons.filled.Search

import androidx.compose.runtime.*

import androidx.lifecycle.compose.collectAsStateWithLifecycle

import androidx.navigation3.runtime.*
import androidx.navigation3.runtime.NavKey
import androidx.navigation3.runtime.NavBackStack

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

import org.koin.compose.koinInject

import me.voltual.rp.KtorClient
import me.voltual.rp.data.UpdateInfo
import me.voltual.rp.data.UpdateSettingsDataStore
import me.voltual.rp.data.UserAgreementDataStore
import me.voltual.rp.core.utils.UpdateCheckResult
import me.voltual.rp.core.utils.UpdateChecker

import me.voltual.rp.core.ui.theme.*
import me.voltual.rp.core.ui.theme.ThemeCustomizeScreen
import me.voltual.rp.core.ui.components.UserAgreementDialog
import me.voltual.rp.core.ui.components.UpdateDialog
import me.voltual.rp.core.ui.animation.*
import me.voltual.rp.ui.*

val topLevelRoutes: Set<NavKey> = setOf(Home)

@Composable
fun PyrolysisApp(
    agreementDataStore: UserAgreementDataStore = koinInject(),
    modifier: Modifier = Modifier,
    platformEntryProvider: @Composable (NavKey, Navigator) -> (@Composable () -> Unit)? = { _, _ -> null }
) {
    val navigationState = rememberNavigationState(
        startRoute = Home,
        topLevelRoutes = topLevelRoutes
    )
    val focusManager = LocalFocusManager.current
    val topAppBarController = remember { TopAppBarController() }
    val navigator = remember(focusManager, topAppBarController, navigationState) {
        Navigator(navigationState, focusManager, topAppBarController)
    }

    CompositionLocalProvider(
        LocalNavigator provides navigator,
        LocalNavigationState provides navigationState,
        LocalTopAppBarController provides topAppBarController,
    ) {
        val snackbarHostState = remember { SnackbarHostState() }

        val userAccepted by agreementDataStore.isUserAgreementAccepted.collectAsState(initial = true)

        var isAgreementDataLoaded by remember { mutableStateOf(false) }
        LaunchedEffect(Unit) {
            delay(150)
            isAgreementDataLoaded = true
        }

        // 弹窗显示条件简化为只判断用户协议是否接受
        val showAgreementDialog = isAgreementDataLoaded && !userAccepted

        BBQTheme() {
            MainScreenContent(
                navigationState = navigationState,
                navigator = navigator,
                snackbarHostState = snackbarHostState,
                showAgreementDialog = showAgreementDialog,
                platformEntryProvider = platformEntryProvider
            )
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreenContent(
    navigationState: NavigationState,
    navigator: Navigator,
    snackbarHostState: SnackbarHostState,
    showAgreementDialog: Boolean,
    platformEntryProvider: @Composable (NavKey, Navigator) -> (@Composable () -> Unit)?
) {
    val drawerState = rememberDrawerState(initialValue = DrawerValue.Closed)
    val scope = rememberCoroutineScope()
    val themeStore: ThemeColorDataStore = koinInject()

    val currentRoute = navigationState.currentRoute
    val currentTopLevelRoute = navigationState.topLevelRoute

    val showBackButton = remember(currentRoute) {
        currentRoute != Home 
    }

    val topAppBarController = LocalTopAppBarController.current

    val useDarkTheme = ThemeManager.isAppDarkTheme
    val lightBgUri by themeStore.drawerHeaderLightBackgroundUriFlow.collectAsState(initial = null)
    val darkBgUri by themeStore.drawerHeaderDarkBackgroundUriFlow.collectAsState(initial = null)
    val drawerHeaderBackgroundUri = if (useDarkTheme) darkBgUri else lightBgUri

    ModalNavigationDrawer(
        drawerState = drawerState,
        drawerContent = {
            Box(modifier = Modifier.width(360.dp)) {
                Column(
                    modifier = Modifier
                        .fillMaxHeight()
                        .roundScreenPadding()
                        .background(MaterialTheme.colorScheme.surfaceVariant)
                ) {
                    DrawerHeader(
                        modifier = Modifier
                            .fillMaxWidth()
                            .height(180.dp),
                        backgroundUri = drawerHeaderBackgroundUri
                    )
                    NavigationDrawerItems(
                        navigator = navigator,
                        currentTopLevelRoute = currentTopLevelRoute,
                        drawerState = drawerState,
                        scope = scope
                    )
                }
            }
        },
        gesturesEnabled = true,
        modifier = Modifier.fillMaxSize()
    ) {
        Scaffold(
            topBar = {
                if (currentRoute != Home) {
                    TopAppBar(
                        title = {
                            Text(
                                text = topAppBarController.customTitle ?: getTitleForDestination(currentRoute),
                                color = MaterialTheme.colorScheme.onSurface,
                                maxLines = 1
                            )
                        },
                        navigationIcon = {
                            if (showBackButton) {
                                IconButton(onClick = { navigator.goBack() }) {
                                    Icon(
                                        imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                                        contentDescription = "返回",
                                        tint = MaterialTheme.colorScheme.onSurface
                                    )
                                }
                            } else {
                                IconButton(onClick = { scope.launch { drawerState.open() } }) {
                                    Icon(
                                        imageVector = Icons.Default.Menu,
                                        contentDescription = "打开菜单",
                                        tint = MaterialTheme.colorScheme.onSurface
                                    )
                                }
                            }
                        },
                        actions = {
                        },
                        colors = TopAppBarDefaults.topAppBarColors(
                            containerColor = MaterialTheme.colorScheme.surface,
                            titleContentColor = MaterialTheme.colorScheme.onSurface
                        )
                    )
                }
            },
            snackbarHost = { BBQSnackbarHost(hostState = snackbarHostState) },
            content = { innerPadding ->
                val contentPadding = if (currentRoute == Home) PaddingValues(0.dp) else innerPadding
                
                val currentBackStack = navigationState.backStacks[currentTopLevelRoute]
                    ?: navigationState.backStacks[navigationState.startRoute]!!

                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(contentPadding)
                        .roundScreenPadding()
                ) {
                    BBQNavDisplay(
                        backStack = currentBackStack,
                        onBack = { navigator.goBack() },
                        snackbarHostState = snackbarHostState,
                        modifier = Modifier.fillMaxSize(),
                        platformEntryProvider = { key ->
                            platformEntryProvider(key, navigator)
                        }
                    )

                    if (currentRoute == Home) {
                        Box(
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(16.dp),
                            contentAlignment = Alignment.TopStart
                        ) {
                            FilledIconButton(
                                onClick = { scope.launch { drawerState.open() } },
                                colors = IconButtonDefaults.filledIconButtonColors(
                                    containerColor = MaterialTheme.colorScheme.surface.copy(alpha = 0.7f),
                                    contentColor = MaterialTheme.colorScheme.onSurface
                                )
                            ) {
                                Icon(
                                    imageVector = Icons.Default.Menu,
                                    contentDescription = "打开菜单"
                                )
                            }
                        }
                    }

                    if (showAgreementDialog) {
                        UserAgreementDialog(
                            onAgreed = { },
                        )
                    }

                    CheckForUpdates(snackbarHostState)
                }
            }
        )
    }
}

@Composable
fun getTitleForDestination(route: NavKey?): String {
    return when (route) {
        Home -> "主页"
        ThemeCustomize -> "主题定制"
        UpdateSettings -> "更新设置"
        else -> "在~ $route ~里~哦"
    }
}

@Composable
fun CheckForUpdates(snackbarHostState: SnackbarHostState) {
    val coroutineScope = rememberCoroutineScope()
    var updateInfo by remember { mutableStateOf<UpdateInfo?>(null) }
    val updateSettingsDataStore: UpdateSettingsDataStore = koinInject()

    var showDialog by remember { mutableStateOf(false) }

    LaunchedEffect(Unit) {
        val autoCheckUpdates = updateSettingsDataStore.autoCheckUpdates.first()
        if (autoCheckUpdates) {
            UpdateChecker.checkForUpdates { result ->
                when (result) {
                    is UpdateCheckResult.Success -> {
                        updateInfo = result.updateInfo
                        showDialog = true
                    }
                    is UpdateCheckResult.NoUpdate -> {
                        coroutineScope.launch {
                            snackbarHostState.showSnackbar("当前已是最新版本")
                        }
                    }
                    is UpdateCheckResult.Error -> {
                        coroutineScope.launch {
                            snackbarHostState.showSnackbar(result.message)
                        }
                    }
                }
            }
        }
    }

    updateInfo?.let { info ->
        if (showDialog) {
            UpdateDialog(updateInfo = info) {
                showDialog = false
                updateInfo = null
            }
        }
    }
}