# Что значит вывод `git status` и как с этим работать

## «Changes not staged for commit»

Это значит: **файлы уже отслеживаются Git’ом**, но **текущее содержимое на диске отличается** от того, что было в **последнем коммите**. Изменения **ещё не добавлены** в «зону» следующего коммита (staging).

- **`git add <файл>`** — положить конкретный файл в staging (он попадёт в следующий коммит).
- **`git add -A`** или **`git add .`** — добавить все изменения отслеживаемых файлов (осторожно: попадёт всё, что не в `.gitignore`).
- **`git restore <файл>`** — **откатить** файл к версии из последнего коммита (твои правки в этом файле пропадут).

Итого: «not staged» = «есть правки, но коммит их ещё не содержит и в staging они не лежат».

---

## Разбор твоих изменённых файлов

| Группа | Файлы | Что это | Обычно в коммит? |
|--------|--------|---------|------------------|
| IDE | `fclient/.idea/deploymentTargetSelector.xml`, `vcs.xml` | Настройки Android Studio | По желанию команды (часто в `.gitignore`) |
| Исходники приложения | `MainActivity.java`, `PinpadActivity.java`, `native-lib.cpp`, layouts, manifest, `build.gradle`, themes, strings… | Твой код и конфиг модуля | **Да** — это то, что обычно коммитят |
| Сборка mbedTLS | `libs/mbedtls/build/compile.bat` | Скрипт сборки | По ситуации |
| **Подмодуль** | `libs/mbedtls/mbedtls (modified content)` | Внутри подмодуля есть незакоммиченные изменения | Отдельно: зайти в подмодуль, там `add`/`commit` или откат |
| **Артефакты сборки spdlog** | `libs/spdlog/build/...` (.ninja_*, CMake*.log и т.д.) | Результат CMake/Ninja | **Обычно не коммитят** — добавить в `.gitignore` |

---

## «Untracked files»

Файлы/папки, которые **Git пока не отслеживает** (никогда не были в коммите или лежат вне отслеживания).

| Путь | Что это | Рекомендация |
|------|---------|--------------|
| `TransactionEvents.java` | Новый код | **`git add`** если нужен в репозитории |
| `Type.kt` | Новый файл | **`git add`** или удалить, если случайно |
| `libs/mbedtls/build/arm64-v8a/` и др. | **Собранные библиотеки** под ABI | **Не коммитить** — добавить в `.gitignore` (как у spdlog) |

---

## Как «исправить» ситуацию (варианты)

### Вариант A — закоммитить осмысленные правки приложения

```bash
cd c:\Users\yan\AndroidStudioProjects\rpo

# Только исходники и конфиг fclient (без .idea и без build-артефактов)
git add fclient/app/src/main/java/ru/iu3/fclient/MainActivity.java
git add fclient/app/src/main/java/ru/iu3/fclient/PinpadActivity.java
git add fclient/app/src/main/java/ru/iu3/fclient/TransactionEvents.java
git add fclient/app/src/main/cpp/native-lib.cpp
git add fclient/app/src/main/cpp/CMakeLists.txt
git add fclient/app/src/main/AndroidManifest.xml
git add fclient/app/src/main/res/
git add fclient/app/build.gradle
git add fclient/build.gradle
git add fclient/gradle/libs.versions.toml
# при необходимости — остальные файлы проекта, которые ты хочешь в истории

git status   # проверить, что в staging только нужное
git commit -m "fclient: транзакция, JNI, правки UI"
```

Подставь свои пути, если что-то не нужно в коммите.

### Вариант B — не коммитить мусор сборки: `.gitignore`

Добавь (или дополни) `.gitignore` в корне репозитория, чтобы Git перестал предлагать эти папки:

```gitignore
# Артефакты нативной сборки (не коммитить)
libs/spdlog/build/
libs/mbedtls/build/arm64-v8a/
libs/mbedtls/build/armeabi-v7a/
libs/mbedtls/build/x86/
libs/mbedtls/build/x86_64/

# По желанию — IDE
# fclient/.idea/deploymentTargetSelector.xml
# или целиком: fclient/.idea/
```

После этого **`git status`** перестанет показывать игнорируемые файлы как modified/untracked (уже отслеживаемые файлы из `libs/spdlog/build/` один раз придётся убрать из индекса, см. ниже).

### Вариант C — убрать из Git то, что уже случайно отслеживалось (build)

Если папки `libs/spdlog/build/...` уже были закоммичены раньше:

```bash
git rm -r --cached libs/spdlog/build
```

`--cached` — убрать только из Git, файлы на диске останутся. Затем коммит + правило в `.gitignore`.

### Вариант D — подмодуль `libs/mbedtls/mbedtls`

Сообщение **«modified content in submodules»** значит: внутри каталога-подмодуля есть свои незакоммиченные изменения.

```bash
cd libs/mbedtls/mbedtls
git status
# либо закоммитить там: git add ... && git commit -m "..."
# либо откатить: git restore .  или  git checkout -- .
cd ../../..
```

Пока подмодуль «грязный», в **родительском** репозитории будет висеть «modified: libs/mbedtls/mbedtls».

### Вариант E — откатить всё, что не хочешь сохранять

Осторожно: потеря правок.

```bash
git restore fclient/.idea/deploymentTargetSelector.xml
git restore libs/spdlog/build/   # если не нужны изменения в логах сборки
# и т.д.
```

---

## Кратко

| Фраза в Git | Смысл |
|-------------|--------|
| **Changes not staged** | Файл отслеживается, на диске другая версия, чем в последнем коммите; `git add` — взять в коммит, `git restore` — выбросить правки. |
| **Untracked** | Git файл не знает; `git add` — начать отслеживать; или добавить путь в `.gitignore`. |
| **submodules modified** | Внутри подмодуля свои коммиты/правки — разруливать **внутри** подмодуля. |

Этот файл можно удалить после того, как разберёшься с репозиторием — он только для справки.
