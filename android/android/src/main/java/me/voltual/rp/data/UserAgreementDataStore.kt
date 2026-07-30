//Copyright (C) 2025 Voltual
// 本程序是自由软件：你可以根据自由软件基金会发布的 GNU 通用公共许可证第3版
//（或任意更新的版本）的条款重新分发和/或修改它。
//本程序是基于希望它有用而分发的，但没有任何担保；甚至没有适销性或特定用途适用性的隐含担保。
// 有关更多细节，请参阅 GNU 通用公共许可证。
//
// 你应该已经收到了一份 GNU 通用公共许可证的副本
// 如果没有，请查阅 <http://www.gnu.org/licenses/>.
package me.voltual.rp.data

import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.*
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

object UserAgreementVersion {
    const val CURRENT = 1
}

class UserAgreementDataStore(private val dataStore: DataStore<Preferences>) {

    private object Keys {
        val USER_AGREEMENT_VER = intPreferencesKey("user_agreement_ver")
    }

    val isUserAgreementAccepted: Flow<Boolean> = dataStore.data.map { prefs ->
        (prefs[Keys.USER_AGREEMENT_VER] ?: 0) >= UserAgreementVersion.CURRENT
    }

    suspend fun acceptUserAgreement() {
        dataStore.edit { prefs ->
            prefs[Keys.USER_AGREEMENT_VER] = UserAgreementVersion.CURRENT
        }
    }
}