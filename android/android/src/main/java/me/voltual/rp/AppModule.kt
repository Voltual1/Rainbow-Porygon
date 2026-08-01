// Copyright (C) 2025 Voltual
// 本程序是自由软件：你可以根据自由软件基金会发布的 GNU 通用公共许可证第3版
// （或任意更新的版本）的条款重新分发和/或修改它。
// 本程序是基于希望它有用而分发的，但没有任何担保；甚至没有适销性或特定用途适用性的隐含担保。
// 有关更多细节，请参阅 GNU 通用公共许可证。
//
// 你应该已经收到了一份 GNU 通用公共许可证的副本
// 如果没有，请查阅 <http://www.gnu.org/licenses/>.
package me.voltual.rp

import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.PreferenceDataStoreFactory
import androidx.datastore.preferences.preferencesDataStoreFile
import me.voltual.rp.core.database.*
import me.voltual.rp.core.database.dao.*
import me.voltual.rp.data.*
import me.voltual.rp.core.ftp.FtpServerManager
import me.voltual.rp.ui.settings.update.*
import org.koin.android.ext.koin.androidApplication
import org.koin.android.ext.koin.androidContext
import org.koin.core.module.dsl.viewModel
import me.voltual.rp.core.database.repository.*
import org.koin.dsl.module
import me.voltual.rp.core.ui.theme.*
import org.koin.core.qualifier.named

val USER_AGREEMENT_STORE_QUALIFIER = named("user_agreement_store")
val UPDATE_SETTINGS_STORE_QUALIFIER = named("update_settings_store")
val DRAWER_MENU_STORE_QUALIFIER = named("drawer_menu_store")
val THEME_SETTINGS_STORE_QUALIFIER = named("theme_settings_store")

val appModule = module {
    viewModel { UpdateSettingsViewModel(get()) }
    single { BBQApplication.instance.database }
    single { get<AppDatabase>().logDao() }
    single { LogRepository(get()) }
      
    // 底层 DataStore<Preferences> 依赖注入定义
    single<DataStore<Preferences>>(USER_AGREEMENT_STORE_QUALIFIER) {
        PreferenceDataStoreFactory.create(
            produceFile = { androidContext().preferencesDataStoreFile("user_agreement") }
        )
    }
    single<DataStore<Preferences>>(UPDATE_SETTINGS_STORE_QUALIFIER) {
        PreferenceDataStoreFactory.create(
            produceFile = { androidContext().preferencesDataStoreFile("update_settings") }
        )
    }
    single<DataStore<Preferences>>(THEME_SETTINGS_STORE_QUALIFIER) {
        PreferenceDataStoreFactory.create(
            produceFile = { androidContext().preferencesDataStoreFile("theme_settings") }
        )
    }
    single<DataStore<Preferences>>(DRAWER_MENU_STORE_QUALIFIER) {
        PreferenceDataStoreFactory.create(
            produceFile = { androidContext().preferencesDataStoreFile("drawer_menu") }
        )
    }
      
    // 业务层 DataStore 包装类
    single { UserAgreementDataStore(get(USER_AGREEMENT_STORE_QUALIFIER)) }
    single { FtpSettingsDataStore(androidContext()) }
    single { FtpServerManager(androidContext(), get()) }
    single { UpdateSettingsDataStore(get(UPDATE_SETTINGS_STORE_QUALIFIER)) }
    single { ThemeColorDataStore(get(THEME_SETTINGS_STORE_QUALIFIER)) }
    single { DrawerMenuDataStore(get(DRAWER_MENU_STORE_QUALIFIER)) }
}