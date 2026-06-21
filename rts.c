#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cJSON.h"

#define ERR1 -1

// Паттерн: CHECK_NULL — если NULL, печатаем и возвращаем
#define CHECK_NULL_ERR1(ptr, msg) \
    do { \
        if ((ptr) == NULL) { \
            printf("%s\n", msg); \
            return ERR1; \
        } \
    } while(0)

#define CHECK_NULL(ptr, msg) \
    do { \
        if ((ptr) == NULL) { \
            printf("%s\n", msg); \
            return NULL; \
        } \
    } while(0)

#define CHECK_NULL_VOID(ptr, msg) \
    do { \
        if ((ptr) == NULL) { \
            printf("%s\n", msg); \
            return; \
        } \
    } while(0)

#define CHECK_NULL_ISNUMBER(ptr, msg) \
    do { \
        if ((ptr) == NULL || !(cJSON_IsNumber(ptr))) { \
            printf("%s\n", msg); \
            return ERR1; \
        } \
    } while(0)


// структура для времени
typedef struct{
	int Year;
	int Month;
	int Day;
} Time;


void save_in_file(cJSON *root, char *name);
cJSON* file_in_json(char *name);
int add_time(cJSON *root);
int up_standart_skill(cJSON *root, char *name, int count);
int fit_time_root(cJSON *root, cJSON *object);

// загрузка из файла в json
cJSON* file_in_json(char *name){
	FILE *fp = fopen(name, "r");
	CHECK_NULL(fp, "Не открывается файл json(1.1)\n");

	fseek(fp, 0, SEEK_END);
	long size_file = ftell(fp);
	rewind(fp);

	char *buffer = (char*)malloc(size_file+1);
	if(buffer == NULL){
		printf("Не выделяется буфер (2)\n");
		fclose(fp);
		return NULL;
	}

	
	size_t read_size = fread(buffer, 1, size_file, fp);

	if(read_size != size_file){
		printf("Файл не прочитан до конца(3)");
		free(buffer);
		fclose(fp);
		return NULL;
	}

	buffer[read_size] = '\0';
	fclose(fp);

	cJSON *result = cJSON_Parse(buffer);
	if(result == NULL){
		//printf("Не парсится в json (4)");
		free(buffer);
		return NULL;
	}

	free(buffer);
	return result;
}




// загрузка из json-структуры в файл
void save_in_file(cJSON *root, char *name){
	FILE *fp = fopen(name, "w");
	CHECK_NULL_VOID(fp, "Не открывается файл json(1.1)\n");

	char *json_txt = cJSON_Print(root);
	if(json_txt == NULL){
		printf("Не парсится в строку(2)\n");
		fclose(fp);
		return;
	}

	fprintf(fp, "%s", json_txt);

	cJSON_free(json_txt);
	fclose(fp);
}

// подсчет дней между датами
int calculate_days_passed(cJSON *last_time, cJSON *current_time) {
    CHECK_NULL_ERR1(last_time, "Не удалось достать last_time в calculate_days_passed");
    CHECK_NULL_ERR1(current_time, "Не удалось достать current_time в calculate_days_passed");
    
    int last_day = cJSON_GetObjectItem(last_time, "DAY")->valueint;
    int last_month = cJSON_GetObjectItem(last_time, "MONTH")->valueint;
    int last_year = cJSON_GetObjectItem(last_time, "YEAR")->valueint;
    
    int curr_day = cJSON_GetObjectItem(current_time, "DAY")->valueint;
    int curr_month = cJSON_GetObjectItem(current_time, "MONTH")->valueint;
    int curr_year = cJSON_GetObjectItem(current_time, "YEAR")->valueint;
    
    // Простой подсчёт (без учёта високосных лет, для простоты)
    int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Переводим в дни с начала года
    int last_total_days = last_day;
    for (int i = 1; i < last_month; i++) {
        last_total_days += days_in_month[i];
    }
    last_total_days += (last_year - 2000) * 365; // Грубый подсчёт
    
    int curr_total_days = curr_day;
    for (int i = 1; i < curr_month; i++) {
        curr_total_days += days_in_month[i];
    }
    curr_total_days += (curr_year - 2000) * 365;
    
    int diff = curr_total_days - last_total_days;
    return diff > 0 ? diff : 0;
}

// Функция тления стихий
void check_skill_decay(cJSON *root, cJSON *skills_group) {
    if (!skills_group) return;
    cJSON *current_time = cJSON_GetObjectItem(root, "time");
    cJSON *skill = skills_group->child;
    
    while (skill) {
        cJSON *skill_time = cJSON_GetObjectItem(skill, "time");
        if (skill_time) {
            int days_passed = calculate_days_passed(skill_time, current_time);
            if (days_passed > 7) {
                cJSON *level = cJSON_GetObjectItem(skill, "level");
                cJSON *xp = cJSON_GetObjectItem(skill, "xp");
                if (level && level->valueint > 0) {
                    cJSON_SetNumberValue(level, level->valueint - 1);
                    cJSON_SetNumberValue(xp, 0);
                    printf("🍂 Стихия '%s' тлеет! Уровень снижен до %d (не использовалась %d дн.)\n", 
                           skill->string, level->valueint, days_passed);
                    // Обновляем время, чтобы не тлела каждый день подряд
                    fit_time_root(root, skill); 
                }
            }
        }
        skill = skill->next;
    }
}

// Хелпер: просто обновить время скилла без прокачки (чтобы не тлел)
void update_skill_time_only(cJSON *root, char *skill_name) {
    cJSON *skills = cJSON_GetObjectItem(root, "skills");
    cJSON *standart = cJSON_GetObjectItem(skills, "standart_skills");
    cJSON *my = cJSON_GetObjectItem(skills, "my_skills");
    
    cJSON *target_skill = cJSON_GetObjectItem(standart, skill_name);
    if (!target_skill) target_skill = cJSON_GetObjectItem(my, skill_name);
    
    if (target_skill) {
        fit_time_root(root, target_skill);
    }
}

// Подсчет общей Силы Королевства
int calculate_kingdom_power(cJSON *root) {
    int power = 0;
    
    // 1. Сумма уровней всех скиллов
    cJSON *skills = cJSON_GetObjectItem(root, "skills");
    cJSON *groups[] = {
        cJSON_GetObjectItem(skills, "standart_skills"),
        cJSON_GetObjectItem(skills, "my_skills")
    };
    
    for (int i = 0; i < 2; i++) {
        if (!groups[i]) continue;
        cJSON *skill = groups[i]->child;
        while (skill) {
            cJSON *lvl = cJSON_GetObjectItem(skill, "level");
            if (lvl) power += lvl->valueint;
            skill = skill->next;
        }
    }
    
    // 2. Артефакты (используем big_achievements как счетчик)
    cJSON *forge = cJSON_GetObjectItem(root, "forge");
    cJSON *arts = cJSON_GetObjectItem(forge, "big_achievements");
    if (arts) power += arts->valueint * 2; // 2 очка силы за артефакт
    
    // 3. Прочитанные книги
    cJSON *library = file_in_json("library.json");
    if (library) {
        cJSON *book = library->child;
        while (book) {
            cJSON *status = cJSON_GetObjectItem(book, "status");
            if (status && strcmp(status->valuestring, "Прочитано") == 0) {
                power += 3; // 3 очка силы за прочитанную книгу
            }
            book = book->next;
        }
        cJSON_Delete(library);
    }
    
    return power;
}

// Масштабирование Моргота при спавне
void scale_morgoth(cJSON *root) {
    int power = calculate_kingdom_power(root);
    int new_max_hp = 500 + (power * 10); // Коэффициент 10, чтобы он не был.immortal
    
    cJSON *morgoth = cJSON_GetObjectItem(root, "morgoth");
    cJSON_SetNumberValue(cJSON_GetObjectItem(morgoth, "max_hp"), new_max_hp);
    cJSON_SetNumberValue(cJSON_GetObjectItem(morgoth, "hp"), new_max_hp); // Полный хил
    
    printf("📈 Сила Королевства: %d. Моргот мутирует! Max HP: %d\n", power, new_max_hp);
}

// Применяем баффы/дебаффы от образования в начале сессии
void apply_education_modifiers(cJSON *root) {
    int player_buff = 0;
    int morgoth_buff = 0;
    
    cJSON *library = file_in_json("library.json");
    if (!library) return;
    
    cJSON *book = library->child;
    while (book) {
        cJSON *status = cJSON_GetObjectItem(book, "status");
        if (status && cJSON_IsString(status)) {
            if (strcmp(status->valuestring, "В процессе чтения") == 0) {
                player_buff += 5; // +5% к твоему урону за каждую активную книгу
            }
            if (strcmp(status->valuestring, "Заброшено") == 0) {
                morgoth_buff += 10; // +10% к урону Моргота за каждую заброшенную
            }
        }
        book = book->next;
    }
    cJSON_Delete(library);
    
    // Сохраняем в дебаффы (используем существующие поля для простоты)
    cJSON *debuffs = cJSON_GetObjectItem(root, "debuffs");
    if (player_buff > 0) {
        printf("🧠 Образование: Ты читаешь книги. Твой урон +%d%%\n", player_buff);
        // Вычтем из твоего damage_reduction (сделаем его отрицательным, если нужно, или добавим новое поле)
        // Для простоты: просто добавим к morgoth_damage_bonus_percent отрицательное значение, 
        // но лучше добавить новые поля. Давай добавим education_buff в debuffs.
    }
    if (morgoth_buff > 0) {
        printf("📉 Невежество: У тебя %d заброшенных книг. Моргот бьет на %d%% сильнее!\n", morgoth_buff / 10, morgoth_buff);
        cJSON *m_bonus = cJSON_GetObjectItem(debuffs, "morgoth_damage_bonus_percent");
        cJSON_SetNumberValue(m_bonus, m_bonus->valueint + morgoth_buff);
    }
}

// открытие сессии
int battle_start(cJSON *root) {
    CHECK_NULL_ERR1(root, "Не удалось достать root в battle_start");
    cJSON *week = cJSON_GetObjectItem(root, "week_battle");
    cJSON *morgoth = cJSON_GetObjectItem(root, "morgoth");

    // 1. Проверяем тление стихий
    cJSON *skills = cJSON_GetObjectItem(root, "skills");
    check_skill_decay(root, cJSON_GetObjectItem(skills, "standart_skills"));
    check_skill_decay(root, cJSON_GetObjectItem(skills, "my_skills"));

    if (cJSON_IsTrue(cJSON_GetObjectItem(week, "active"))) {
        cJSON_ReplaceItemInObject(week, "active_now", cJSON_CreateBool(true));
        
        // Проверяем пропуск
        cJSON *last_session = cJSON_GetObjectItem(week, "last_session_time");
        cJSON *current_time = cJSON_GetObjectItem(root, "time");
        int days_passed = calculate_days_passed(last_session, current_time);
        
        if (days_passed > 1) {
            printf("⚠️ ПРОПУСК ОБНАРУЖЕН! Ты отсутствовал %d дн.\n", days_passed);
            cJSON *debuffs = cJSON_GetObjectItem(root, "debuffs");
            
            // Моргот отхиливается
            int heal_amount = days_passed * 20;
            cJSON *hp = cJSON_GetObjectItem(morgoth, "hp");
            cJSON *max_hp = cJSON_GetObjectItem(morgoth, "max_hp");
            int new_hp = hp->valueint + heal_amount;
            if (new_hp > max_hp->valueint) new_hp = max_hp->valueint;
            cJSON_SetNumberValue(hp, new_hp);
            printf("🐺 Моргот отхилился на %d HP (HP: %d/%d)\n", heal_amount, new_hp, max_hp->valueint);
            
            // Дебаффы за пропуск
            cJSON *damage_reduction = cJSON_GetObjectItem(debuffs, "damage_reduction_percent");
            int new_reduction = damage_reduction->valueint + (days_passed * 10);
            if (new_reduction > 50) new_reduction = 50;
            cJSON_SetNumberValue(damage_reduction, new_reduction);
            printf("⚔️ Твой урон снижен на %d%% (дебафф за пропуск)\n", new_reduction);
            
            cJSON *morgoth_bonus = cJSON_GetObjectItem(debuffs, "morgoth_damage_bonus_percent");
            int new_bonus = morgoth_bonus->valueint + (days_passed * 10);
            if (new_bonus > 50) new_bonus = 50;
            cJSON_SetNumberValue(morgoth_bonus, new_bonus);
            printf("💀 Урон Моргота увеличен на %d%% (дебафф за пропуск)\n", new_bonus);
        }
    } else {
        // Новая битва -> Масштабируем Моргота!
        scale_morgoth(root);
        
        cJSON_ReplaceItemInObject(week, "active", cJSON_CreateBool(true));
        cJSON_ReplaceItemInObject(week, "active_now", cJSON_CreateBool(true));
        cJSON_SetNumberValue(cJSON_GetObjectItem(week, "sessions_count"), 0);
        cJSON_SetNumberValue(cJSON_GetObjectItem(week, "total_damage_dealt"), 0);
        cJSON_SetNumberValue(cJSON_GetObjectItem(week, "total_damage_taken"), 0);
        
        cJSON *time = cJSON_GetObjectItem(root, "time");
        cJSON *start_time = cJSON_GetObjectItem(week, "start_time");
        cJSON_SetNumberValue(cJSON_GetObjectItem(start_time, "DAY"), cJSON_GetObjectItem(time, "DAY")->valueint);
        cJSON_SetNumberValue(cJSON_GetObjectItem(start_time, "MONTH"), cJSON_GetObjectItem(time, "MONTH")->valueint);
        cJSON_SetNumberValue(cJSON_GetObjectItem(start_time, "YEAR"), cJSON_GetObjectItem(time, "YEAR")->valueint);
        
        printf("⚔️ КАМПАНИЯ НАЧАТА!\n");
    }

    // 2. Применяем механику образования
    apply_education_modifiers(root);

    // Обновляем last_session_time
    cJSON *time = cJSON_GetObjectItem(root, "time");
    cJSON *last_session = cJSON_GetObjectItem(week, "last_session_time");
    cJSON_SetNumberValue(cJSON_GetObjectItem(last_session, "DAY"), cJSON_GetObjectItem(time, "DAY")->valueint);
    cJSON_SetNumberValue(cJSON_GetObjectItem(last_session, "MONTH"), cJSON_GetObjectItem(time, "MONTH")->valueint);
    cJSON_SetNumberValue(cJSON_GetObjectItem(last_session, "YEAR"), cJSON_GetObjectItem(time, "YEAR")->valueint);

    printf("🐺 HP Моргота: %d/%d\n", cJSON_GetObjectItem(morgoth, "hp")->valueint, cJSON_GetObjectItem(morgoth, "max_hp")->valueint);
    printf("🦌 HP Цитадели: %d/150\n", cJSON_GetObjectItem(root, "health_point")->valueint);
    
    save_in_file(root, "ikingdom.json");
    return 1;
}


// моя атака
int battle_attack(cJSON *root, char *action_type, int hours) {
    CHECK_NULL_ERR1(root, "Не удалось достать root в battle_attack");
    cJSON *week = cJSON_GetObjectItem(root, "week_battle");
    if (!cJSON_IsTrue(cJSON_GetObjectItem(week, "active_now"))) {
        printf("❌ Сессия не активна!\n");
        return ERR1;
    }

    int base_damage = hours * 15;
    
    // 1. Бафф от образования (активно читаемые книги)
    int edu_buff = 0;
    cJSON *library = file_in_json("library.json");
    if (library) {
        cJSON *book = library->child;
        while (book) {
            cJSON *status = cJSON_GetObjectItem(book, "status");
            if (status && strcmp(status->valuestring, "В процессе чтения") == 0) edu_buff += 5;
            book = book->next;
        }
        cJSON_Delete(library);
    }
    base_damage = base_damage * (100 + edu_buff) / 100;

    // 2. Дебафф за пропуск
    cJSON *debuffs = cJSON_GetObjectItem(root, "debuffs");
    int reduction = cJSON_GetObjectItem(debuffs, "damage_reduction_percent")->valueint;
    int final_damage = base_damage * (100 - reduction) / 100;

    // Наносим урон
    cJSON *morgoth = cJSON_GetObjectItem(root, "morgoth");
    cJSON *hp = cJSON_GetObjectItem(morgoth, "hp");
    int new_hp = hp->valueint - final_damage;
    if (new_hp < 0) new_hp = 0;
    cJSON_SetNumberValue(hp, new_hp);

    cJSON *total_dealt = cJSON_GetObjectItem(week, "total_damage_dealt");
    cJSON_SetNumberValue(total_dealt, total_dealt->valueint + final_damage);

    printf("⚔️ Ты атакуешь (%s, %d часов)!\n", action_type, hours);
    printf("💥 Урон: %d", final_damage);
    if (reduction > 0) printf(" (дебафф -%d%%)", reduction);
    if (edu_buff > 0) printf(" (книги +%d%%)", edu_buff);
    printf("\n");
    printf("🐺 HP Моргота: %d/%d\n", new_hp, cJSON_GetObjectItem(morgoth, "max_hp")->valueint);

    // 3. Обновляем время скилла, чтобы он не тлел (без прокачки!)
    if (strcmp(action_type, "train") == 0) update_skill_time_only(root, "focus");
    else if (strcmp(action_type, "code") == 0) update_skill_time_only(root, "discipline");
    else if (strcmp(action_type, "read") == 0) update_skill_time_only(root, "structure");

    save_in_file(root, "ikingdom.json");
    return 1;
}

// атака моргота
int battle_morgoth_attack(cJSON *root, char *attack_type, int hours) {
    CHECK_NULL_ERR1(root, "Не удалось достать root в battle_morgoth_attack");
    cJSON *week = cJSON_GetObjectItem(root, "week_battle");
    if (!cJSON_IsTrue(cJSON_GetObjectItem(week, "active_now"))) {
        printf("❌ Сессия не активна!\n");
        return ERR1;
    }

    int base_damage = hours * 10;

    // 1. Бонус Моргота от дебаффов (пропуск + заброшенные книги)
    cJSON *debuffs = cJSON_GetObjectItem(root, "debuffs");
    int bonus = cJSON_GetObjectItem(debuffs, "morgoth_damage_bonus_percent")->valueint;
    int damage_with_bonus = base_damage * (100 + bonus) / 100;

    // 2. Броня Кузни (Артефакты снижают урон)
    cJSON *forge = cJSON_GetObjectItem(root, "forge");
    int artifacts_count = cJSON_GetObjectItem(forge, "big_achievements")->valueint;
    int armor_percent = artifacts_count * 2; // 2% за каждый артефакт
    if (armor_percent > 50) armor_percent = 50; // Кап 50%
    
    int final_damage = damage_with_bonus * (100 - armor_percent) / 100;

    // Наносим урон Цитадели
    cJSON *citadel_hp = cJSON_GetObjectItem(root, "health_point");
    int new_hp = citadel_hp->valueint - final_damage;
    if (new_hp < 0) new_hp = 0;
    cJSON_SetNumberValue(citadel_hp, new_hp);

    cJSON *total_taken = cJSON_GetObjectItem(week, "total_damage_taken");
    cJSON_SetNumberValue(total_taken, total_taken->valueint + final_damage);

    printf("🐺 Моргот атакует (%s, %d часов)!\n", attack_type, hours);
    printf("💔 Урон по Цитадели: %d", final_damage);
    if (bonus > 0) printf(" (бонус Моргота +%d%%)", bonus);
    if (armor_percent > 0) printf(" (Броня Кузни -%d%%)", armor_percent);
    printf("\n");
    printf("🦌 HP Цитадели: %d/150\n", new_hp);

    save_in_file(root, "ikingdom.json");
    return 1;
}

// закрытие сессии
int battle_end(cJSON *root) {
    CHECK_NULL_ERR1(root, "Не удалось достать root в battle_end");
    
    cJSON *week = cJSON_GetObjectItem(root, "week_battle");
    if (!cJSON_IsTrue(cJSON_GetObjectItem(week, "active"))) {
        printf("❌ Недельная битва не активна!\n");
        return ERR1;
    } else if(!cJSON_IsTrue(cJSON_GetObjectItem(week, "active_now"))){
				printf("❌ Битва не активна!\n");
				return ERR1;
		}
    
    cJSON *morgoth = cJSON_GetObjectItem(root, "morgoth");
    cJSON *morgoth_hp = cJSON_GetObjectItem(morgoth, "hp");
    cJSON *citadel_hp = cJSON_GetObjectItem(root, "health_point");
    
    // Увеличиваем счётчик сессий
    cJSON *sessions = cJSON_GetObjectItem(week, "sessions_count");
    cJSON_SetNumberValue(sessions, sessions->valueint + 1);
    
    printf("\n📊 ИТОГ СЕССИИ #%d\n", sessions->valueint);
    printf("💥 Нанесено урона: %d\n", cJSON_GetObjectItem(week, "total_damage_dealt")->valueint);
    printf("💔 Получено урона: %d\n", cJSON_GetObjectItem(week, "total_damage_taken")->valueint);
    printf("🐺 HP Моргота: %d/%d\n", morgoth_hp->valueint, cJSON_GetObjectItem(morgoth, "max_hp")->valueint);
    printf("🦌 HP Цитадели: %d/150\n\n", citadel_hp->valueint);

		// закрываем дебаффы, чтобы не длились бесконечно
		cJSON *debuffs = cJSON_GetObjectItem(root, "debuffs");

    cJSON *damage_reduction = cJSON_GetObjectItem(debuffs, "damage_reduction_percent");
		int new_reduction = 0;
    cJSON_SetNumberValue(damage_reduction, new_reduction);

		cJSON *morgoth_bonus = cJSON_GetObjectItem(debuffs, "morgoth_damage_bonus_percent");
    int new_bonus = 0;
    cJSON_SetNumberValue(morgoth_bonus, new_bonus);

		// ставим active_now на false
    cJSON_ReplaceItemInObject(week, "active_now", cJSON_CreateBool(false));

    // Проверяем победу
    if (morgoth_hp->valueint <= 0) {
        printf("🏆 ПОБЕДА! Моргот отступает!\n");
        cJSON_SetValuestring(cJSON_GetObjectItem(morgoth, "status"), "retreated");
        
        // Устанавливаем время отступления (2-3 дня)
        cJSON *time = cJSON_GetObjectItem(root, "time");
        cJSON *retreat = cJSON_GetObjectItem(morgoth, "retreat_until");
        int current_day = cJSON_GetObjectItem(time, "DAY")->valueint;
        int current_month = cJSON_GetObjectItem(time, "MONTH")->valueint;
        int current_year = cJSON_GetObjectItem(time, "YEAR")->valueint;
        
        int retreat_days = 2 + (rand() % 2); // 2 или 3 дня
        int new_day = current_day + retreat_days;
        int new_month = current_month;
        int new_year = current_year;
        
        if (new_day > 30) {
            new_day -= 30;
            new_month++;
            if (new_month > 12) {
                new_month = 1;
                new_year++;
            }
        }
        
        cJSON_SetNumberValue(cJSON_GetObjectItem(retreat, "DAY"), new_day);
        cJSON_SetNumberValue(cJSON_GetObjectItem(retreat, "MONTH"), new_month);
        cJSON_SetNumberValue(cJSON_GetObjectItem(retreat, "YEAR"), new_year);
        
        printf("🛡️ Отдыхай до %d.%d.%d\n", new_day, new_month, new_year);
        
        // Сбрасываем дебаффы
        cJSON *debuffs = cJSON_GetObjectItem(root, "debuffs");
        cJSON_SetNumberValue(cJSON_GetObjectItem(debuffs, "damage_reduction_percent"), 0);
        cJSON_SetNumberValue(cJSON_GetObjectItem(debuffs, "morgoth_damage_bonus_percent"), 0);
        
        // Деактивируем неделю
        cJSON_ReplaceItemInObject(week, "active", cJSON_CreateBool(false));
        
    } else if (citadel_hp->valueint <= 0) {
        printf("💀 ПОРАЖЕНИЕ! Моргот торжествует!\n");
        
        // Применяем дебаффы
        cJSON *debuffs = cJSON_GetObjectItem(root, "debuffs");
        int current_reduction = cJSON_GetObjectItem(debuffs, "damage_reduction_percent")->valueint;
        cJSON_SetNumberValue(cJSON_GetObjectItem(debuffs, "damage_reduction_percent"), current_reduction + 20);
        cJSON_SetNumberValue(cJSON_GetObjectItem(debuffs, "morgoth_heal_amount"), 50);
        
        printf("⚠️ Дебаффы на следующую сессию:\n");
        printf("   - Урон снижен на %d%%\n", current_reduction + 20);
        printf("   - Моргот отхилится на 50 HP\n");
        
        // Восстанавливаем HP Цитадели (частично)
        cJSON_SetNumberValue(citadel_hp, 50);
        printf("🦌 Цитадель частично восстановлена: 50/150\n");
        
    } else {
        printf("⏳ Битва продолжается...\n");
    }
    
    save_in_file(root, "ikingdom.json");
    return 1;
}


// добавление полей для стихий
int add_field_elements(cJSON *object){
	CHECK_NULL_ERR1(object, "Не удалось принять структуру json в add_field_elements\n");

	cJSON_AddItemToObject(object, "level", cJSON_CreateNumber(0));
	cJSON_AddItemToObject(object, "xp", cJSON_CreateNumber(0));
	cJSON_AddItemToObject(object, "threshold_next", cJSON_CreateNumber(100));
	if(add_time(object) < 0){
		printf("Не удалось добавить полю стихии время\n");
		return ERR1;
	}
	
	return 1;
}

// добавление стандартных стихий
int add_standart_elements(cJSON *skills){
	CHECK_NULL_ERR1(skills, "Не удалось принять структуру json в add_standart_elements\n");

	char *names[] = {"focus", "discipline", "structure", "resilience"};
	int size = sizeof(names) / sizeof(names[0]);

	for(int i = 0; i < size; i++){
		cJSON *object = cJSON_CreateObject();
		cJSON_AddItemToObject(skills, names[i], object);
		if(add_field_elements(object) < 0){
			printf("Не удалось добавить поле функЦИЕЙ add_field_standart_elements\n");
			return ERR1;
		}
	}
		
	return 1;
}

// добавление времени объекту
int add_time(cJSON *root){
	CHECK_NULL_ERR1(root, "Не удалось принять структуру json в add_time\n");

	// добавление времени
	cJSON *time = cJSON_CreateObject();
	CHECK_NULL_ERR1(time, "не удалось создать объект time\n");
	cJSON_AddItemToObject(root, "time", time);

	cJSON_AddItemToObject(time, "DAY", cJSON_CreateNumber(0));
	cJSON_AddItemToObject(time, "MONTH", cJSON_CreateNumber(0));
	cJSON_AddItemToObject(time, "YEAR", cJSON_CreateNumber(0));
	return 1;
}

// установка времени. дают структуру в которой есть структура time, достаем поля
int set_day(cJSON *root, Time time){
	CHECK_NULL_ERR1(root, "Не удалось принять структуру json в set_day\n");
	
	cJSON *obj_time = cJSON_GetObjectItem(root, "time");
	CHECK_NULL_ERR1(obj_time, "Не удалось достать объект time в set_day\n");

	cJSON *obj_day = cJSON_GetObjectItem(obj_time, "DAY");
	CHECK_NULL_ISNUMBER(obj_day, "Не удалось вытащить DAY\n");
	cJSON_SetNumberValue(obj_day, time.Day);

	//printf("time.Day: %d\n", time.Day);
	//printf("DAY: %d\n", obj_day->valueint);

	cJSON *obj_month = cJSON_GetObjectItem(obj_time, "MONTH");
	CHECK_NULL_ISNUMBER(obj_month, "Не удалось вытащить MONTH\n");
	cJSON_SetNumberValue(obj_month, time.Month);

	//printf("time.Month: %d\n", time.Month);
	//printf("MONTH: %d\n", obj_month->valueint);

	cJSON *obj_year = cJSON_GetObjectItem(obj_time, "YEAR");
	CHECK_NULL_ISNUMBER(obj_year, "Не удалось вытащить YEAR\n");
	cJSON_SetNumberValue(obj_year, time.Year);
	
	//printf("time.Year: %d\n", time.Year);
	//printf("YEAR: %d\n", obj_year->valueint);

	return 1;
}


// создание своих скиллов
int set_skills(cJSON *root, char *name){
	CHECK_NULL_ERR1(root, "Не удалось вытащить структуру root в set_skills\n");
	CHECK_NULL_ERR1(name, "Не удалось вытащить имя скилла в set_skills\n");
	
	cJSON *skills = cJSON_GetObjectItem(root, "skills");
	CHECK_NULL_ERR1(skills, "Не удалось вытащить skills в set_skills\n");

	cJSON *my_skills = cJSON_GetObjectItem(skills, "my_skills");
	CHECK_NULL_ERR1(my_skills, "Не удалось вытащить my_skills в set_skills\n");

	cJSON *new_skills = cJSON_CreateObject();
	CHECK_NULL_ERR1(new_skills, "Не удалось создать new_skills в set_skills\n");

	if(add_field_elements(new_skills) < 0){
		printf("Не удалось добавить поля объекту new_skills в set_skills\n");
		return ERR1;
	}

	cJSON_AddItemToObject(my_skills, name, new_skills);
	return 1;
}

// подгонка времени с главной структуры в активные 
int fit_time_root(cJSON *root, cJSON *object){
	CHECK_NULL_ERR1(root, "Не удается достать root в fit_time_root\n");
	CHECK_NULL_ERR1(object, "Не удается достать object в fit_time_root\n");

	// достаем структуру time из root'a 
	cJSON *time = cJSON_GetObjectItem(root, "time");
	CHECK_NULL_ERR1(time, "Не удается достать time в fit_time_root\n");
	
	cJSON *day = cJSON_GetObjectItem(time, "DAY");
	CHECK_NULL_ERR1(day, "Не удается достать day в fit_time_root\n");
	int iday = day->valueint;


	cJSON *month = cJSON_GetObjectItem(time, "MONTH");
	CHECK_NULL_ERR1(month, "Не удается достать month в fit_time_root\n");
	int imonth = month->valueint;

	
	cJSON *year = cJSON_GetObjectItem(time, "YEAR");
	CHECK_NULL_ERR1(year, "Не удается достать year в fit_time_root\n");
	int iyear = year->valueint;


	// достаем структуру time из объекта
	cJSON *obj_time = cJSON_GetObjectItem(object, "time");
	CHECK_NULL_ERR1(obj_time, "Не удается достать obj_time в fit_time_root\n");

	cJSON *obj_day = cJSON_GetObjectItem(obj_time, "DAY");
	CHECK_NULL_ERR1(obj_day, "Не удается достать obj_day в fit_time_root\n");
	if(obj_day->valueint != iday) cJSON_SetNumberValue(obj_day, iday);

	cJSON *obj_month = cJSON_GetObjectItem(obj_time, "MONTH");
	CHECK_NULL_ERR1(obj_month, "Не удается достать obj_month в fit_time_root\n");
	if(obj_month->valueint != imonth) cJSON_SetNumberValue(obj_month, imonth);
	
	cJSON *obj_year = cJSON_GetObjectItem(obj_time, "YEAR");
	CHECK_NULL_ERR1(obj_year, "Не удается достать obj_year в fit_time_root\n");
	if(obj_year->valueint != iyear) cJSON_SetNumberValue(obj_year, iyear);
	return 1;
		
}

// математическая функция прокачки скилла
int math_up(cJSON *skill, int count){
	CHECK_NULL_ERR1(skill, "Не удается достать skill в math_up\n");
	
	cJSON *xp = cJSON_GetObjectItem(skill, "xp");
	CHECK_NULL_ISNUMBER(xp, "Не удается достать xp в math_up\n");
	int ixp = xp->valueint;

	cJSON *level = cJSON_GetObjectItem(skill, "level");
	CHECK_NULL_ISNUMBER(level, "Не удается достать level в math_up\n");
	int ilevel = level->valueint;

	cJSON *thrhold_next = cJSON_GetObjectItem(skill, "threshold_next");
	CHECK_NULL_ISNUMBER(thrhold_next, "Не удается достать threshold_next в math_up\n");
	int ith_next = thrhold_next->valueint;


	// математические вычисления
	ixp = ixp + count;
	while(1){
		if(ixp >= ith_next) ilevel++;
		else break;
		ixp = ixp - ith_next;
		ith_next += 100;
	}

	// заменяем 
	cJSON_SetNumberValue(xp, ixp);
	cJSON_SetNumberValue(thrhold_next, ith_next);
	cJSON_SetNumberValue(level, ilevel);
	return 1;
}

// функция прокачки стандартных скиллов
int up_standart_skill(cJSON *root, char *name, int count){
	CHECK_NULL_ERR1(root, "Не удалось вытащить структуру root в up_standart_skill\n");
	CHECK_NULL_ERR1(name, "Не удалось вытащить имя скилла в up_standart_skill\n");
  if(count <= 0){
		printf("Колво очков прокачки скилла не может быть равно 0\n");
		return ERR1;
	}

	cJSON *skills = cJSON_GetObjectItem(root, "skills");
	CHECK_NULL_ERR1(skills, "Не удалось вытащить skills в up_standart_skill\n");

	cJSON *standart_skills = cJSON_GetObjectItem(skills, "standart_skills");
	CHECK_NULL_ERR1(standart_skills, "Не удалось вытащить standart_skills в up_standart_skill\n");
	
	cJSON *up_skill = cJSON_GetObjectItem(standart_skills, name);
	CHECK_NULL_ERR1(up_skill, "Не удалось вытащить up_скилл в up_standart_skill\n");
	
	// осталось добавить проверку прокачки скилла
	if(math_up(up_skill, count) < 0){
		printf("Не удалось прокачать скилл %s\n", name);
		return ERR1;
	}

	// подтяжка времени из главной структуры
	// root, объект
	if(fit_time_root(root, up_skill) < 0){
		printf("Не удалось подтянуть время в прокачке стандартного скилла\n");
		return ERR1;
	}
	return 1;
}

// функция прокачки собственных скиллов
int up_skills(cJSON *root, char *name, int count){
	CHECK_NULL_ERR1(root, "Не удалось вытащить структуру root в up_skill\n");
	CHECK_NULL_ERR1(name, "Не удалось вытащить имя скилла в up_skill\n");
	if(count <= 0){
		printf("Колво очков прокачки скилла не может быть равно 0\n");
		return ERR1;
	}
	
	cJSON *skills = cJSON_GetObjectItem(root, "skills");
	CHECK_NULL_ERR1(skills, "Не удалось вытащить skills в up_skill\n");

	cJSON *my_skills = cJSON_GetObjectItem(skills, "my_skills");
	CHECK_NULL_ERR1(my_skills, "Не удалось вытащить my_skills в up_skill\n");
	
	cJSON *up_skill = cJSON_GetObjectItem(my_skills, name);
	CHECK_NULL_ERR1(up_skill, "Не удалось вытащить up_скилл в up_skill\n");
	// осталось добавить проверку прокачки скилла
	if(math_up(up_skill, count) < 0){
		printf("Не удалось прокачать скилл %s\n", name);
		return ERR1;
	}

	// подтяжка времени из главной структуры
	// root, объект
	if(fit_time_root(root, up_skill) < 0){
		printf("Не удалось подтянуть время в прокачке стандартного скилла\n");
		return ERR1;
	}
	return 1;
}

// работа с кузней
int add_big_ach(cJSON *root, int count){
	CHECK_NULL_ERR1(root, "Не удалось достать структуру root в add_big_ach\n");
	if(count <= 0){
		printf("Достижений не может быть 0\n");
		return ERR1;
	}
	
	cJSON *forge = cJSON_GetObjectItem(root, "forge");
	CHECK_NULL_ERR1(forge, "Не удалось достать структуру forge в add_big_ach\n");
	
	cJSON *big_achievements = cJSON_GetObjectItem(forge, "big_achievements");
	CHECK_NULL_ISNUMBER(big_achievements, "Не удалось достать big_achievements\n");
	int count_forge = big_achievements->valueint;

	count_forge += count;
	cJSON_SetNumberValue(big_achievements, count_forge);

	if(fit_time_root(root, forge) < 0){
		printf("Не удалось добавить время в кузню\n");
		return ERR1;
	}

	return 1;
}

// создание библиотеки
cJSON* create_library(){
	cJSON *library = file_in_json("library.json");
	if(library == NULL){
		library = cJSON_CreateObject();
		CHECK_NULL(library, "Не удалось создать библиотеку\n");
		save_in_file(library, "library.json");
		return library;
	}

	return library;
}

// добавление книги в библиотеку
int add_book(cJSON *l_root, char *book_name, char *author, int pages){
	// проверка на нулл поинтеры
	CHECK_NULL_ERR1(l_root, "Библиотека не найдена в add_book\n");
	CHECK_NULL_ERR1(book_name, "Не прочитано название книги\n");
	CHECK_NULL_ERR1(author, "Не прочитано имя автора\n");
	if(pages <= 0){
		printf("у книги не может быть 0 страниц\n");
		return ERR1;
	}

	// создание книги - структуры
	cJSON *book_obj = cJSON_CreateObject();
	CHECK_NULL_ERR1(book_obj, "Не удалось создать объект книгу\n");

	// добавление автора
	cJSON *author_obj = cJSON_CreateString(author);
	if(author_obj == NULL){
		printf("Не удалось создать объект автор\n");
		cJSON_Delete(book_obj);
		return ERR1;
	}
	cJSON_AddItemToObject(book_obj, "author", author_obj);
	
	// добавление всеx страниц
	cJSON *pages_obj = cJSON_CreateNumber(pages);
	if(pages_obj == NULL){
		printf("Не удалось создать объект страницы\n");
		cJSON_Delete(book_obj);
		cJSON_Delete(author_obj);
		return ERR1;
	}
	cJSON_AddItemToObject(book_obj, "pages", pages_obj);

	// добавление прочитанных страниц
	cJSON_AddItemToObject(book_obj, "reading_pages", cJSON_CreateNumber(0));
	// добавление статуса прочитанности (не читал, читаю, прочитано (потом еще добавится с патчем ОБРАЗОВАНИЯ))
	cJSON_AddItemToObject(book_obj, "status", cJSON_CreateString("Еще не читал"));

	if(add_time(book_obj) < 0){
		printf("Не удалось добавить время книге\n");
		cJSON_Delete(book_obj);
	}

	// добавление самой структуры книги
	cJSON_AddItemToObject(l_root, book_name, book_obj);
	save_in_file(l_root, "library.json");
	return 1;
}	

// функция прочитывания книги(root для подгонки времени)
int reading_book(cJSON *root, cJSON *l_root, char *name, int count_pages){
	CHECK_NULL_ERR1(l_root, "Не прочитан l_root в reading_book\n");
	CHECK_NULL_ERR1(name, "Не прочитано имя книги в reading_book\n");
	if(count_pages <= 0){
		printf("Ты не можешь !прочитать! 0 страниц\n");
		return ERR1;
	}
	
	// достаем книгу
	cJSON *book_obj = cJSON_GetObjectItem(l_root, name);
	CHECK_NULL_ERR1(book_obj, "Не удалось найти такую книгу\n");

	// достаем поле колво страниц, колво прочитанных и статус
	cJSON *pages_obj = cJSON_GetObjectItem(book_obj, "pages");
	CHECK_NULL_ISNUMBER(pages_obj, "Не удалось достать объект pages\n");

	cJSON *rdpages_obj = cJSON_GetObjectItem(book_obj, "reading_pages");
	CHECK_NULL_ISNUMBER(rdpages_obj, "Не удалось достать объект reading_pages\n");

	cJSON *status_obj = cJSON_GetObjectItem(book_obj, "status");
	if(status_obj == NULL && !(cJSON_IsString(status_obj))){
		printf("Не удалось достать объект status\n");
		return ERR1;
	}

	int all_pages = pages_obj->valueint;
	int read_pages = rdpages_obj->valueint;

	// сначала проверим на полную прочитанность книги
	if(strcmp("Прочитано", status_obj->valuestring) == 0){
		printf("Вы уже прочитали книгу\n");
		return 1;
	}

	// если не прочитано, добавляем и потом проверяем
	read_pages += count_pages;
	cJSON_SetNumberValue(rdpages_obj, read_pages);
	//подгон времени
	if(fit_time_root(root, book_obj) < 0){
		printf("Не удалось подогнать время в книге\n");
		return ERR1;
	}	
	// если перевалило, то прочитано
	if(read_pages >= all_pages){
		printf("Вы прочитали книгу %s!\n", name);
		cJSON_SetValuestring(status_obj, "Прочитано");
		save_in_file(l_root, "library.json");
		return 1;
	}
	// если же нет, то в процессе чтения
	cJSON_SetValuestring(status_obj, "В процессе чтения");
	save_in_file(l_root, "library.json");
	return 1;
}

// Функция заброса книги
int abandon_book(cJSON *l_root, char *name) {
    CHECK_NULL_ERR1(l_root, "Библиотека не найдена в abandon_book");
    cJSON *book_obj = cJSON_GetObjectItem(l_root, name);
    CHECK_NULL_ERR1(book_obj, "Книга не найдена");
    
    cJSON *status = cJSON_GetObjectItem(book_obj, "status");
    if (strcmp(status->valuestring, "Прочитано") == 0) {
        printf("Нельзя забросить прочитанную книгу!\n");
        return ERR1;
    }
    
    cJSON_SetValuestring(status, "Заброшено");
    save_in_file(l_root, "library.json");
    printf("📉 Книга '%s' заброшена. Моргот рад.\n", name);
    return 1;
}

// установка цели
int set_purpose(cJSON *root, char *name_purpose, char *rank_purpose){
	CHECK_NULL_ERR1(root, "Не удалось принять структуру json в set_purpose\n");
	CHECK_NULL_ERR1(name_purpose, "Не удалось принять имя цели в set_purpose\n");
	CHECK_NULL_ERR1(rank_purpose, "Не удалось принять ранг цели в set_purpose\n");

	cJSON *hunter_table = cJSON_GetObjectItem(root, "hunter_table");
	CHECK_NULL_ERR1(hunter_table, "Не удалось достать структуру json в set_purpose\n");

	cJSON *purpose = cJSON_CreateObject();
	CHECK_NULL_ERR1(purpose, "Не удалось создать структуру json в set_purpose\n");
	
	cJSON_AddItemToObject(purpose, "rank", cJSON_CreateString(rank_purpose));
	cJSON_AddItemToObject(hunter_table, name_purpose, purpose);
	save_in_file(root, "ikingdom.json");
	return 1;
}

// добавление с нуля, возвращает -1 если плохо все
int create_ikingdom(cJSON *root){
	CHECK_NULL_ERR1(root, "Не удалось принять структуру json в create_ikingdom\n");

	if(add_time(root) < 0){
		printf("Не удалось создать время\n");
		return ERR1;
	}
	
	// добавление хп
	cJSON *health_point = cJSON_CreateNumber(150);
	CHECK_NULL_ERR1(health_point, "не удалось создать объект health_point\n");
	cJSON_AddItemToObject(root, "health_point", health_point);

	// общая вкладка скиллов
	cJSON *skills = cJSON_CreateObject();
	CHECK_NULL_ERR1(skills, "не удалось создать объект skills\n");

	// вкладка стандартных скиллов
	cJSON *standart_skills = cJSON_CreateObject();
	if(standart_skills == NULL){
		printf("не удалось создать объект standart_skills\n");
		cJSON_Delete(skills);
		return ERR1;
	}
	// добавление стандартных скиллов
	if(add_standart_elements(standart_skills) < 0){
		printf("не удалось добавить объект В standart_skills с помощью add_standart_elements()\n");
		cJSON_Delete(skills);
		return ERR1;
	}
	cJSON_AddItemToObject(skills, "standart_skills", standart_skills);
	
	// вкладка добавочных скиллов
	cJSON *my_skills = cJSON_CreateObject();
	if(my_skills == NULL){
		printf("не удалось создать объект my_skills\n");
		cJSON_Delete(skills);
		return ERR1;
	}
	cJSON_AddItemToObject(skills, "my_skills", my_skills);
	cJSON_AddItemToObject(root, "skills", skills);
			

	// создаем и добавляем кузню
	cJSON *forge = cJSON_CreateObject();
	CHECK_NULL_ERR1(forge, "не удалось создать объект forge\n");
	cJSON_AddItemToObject(root, "forge", forge);

	cJSON *forge_count = cJSON_CreateNumber(0);
	CHECK_NULL_ERR1(forge_count, "Не удалось создать объект *forge_count\n");
	
	cJSON_AddItemToObject(forge, "big_achievements", forge_count);
	if(add_time(forge) < 0){
		printf("Не удалось добавить время кузнице\n");
		return ERR1;
	}

	// создаем стол контрактов
	cJSON *hunter_table = cJSON_CreateObject();
	CHECK_NULL_ERR1(hunter_table, "Не удалось создать стол контрактов\n");
	
	cJSON_AddItemToObject(root, "hunter_table", hunter_table);
	if(add_time(hunter_table) < 0){
		printf("Не удалось добавить время столку контрактов\n");
		return ERR1;
	}

	// создаем поля о "недельной" битве	
cJSON *week_battle = cJSON_CreateObject();
CHECK_NULL_ERR1(week_battle, "Не удалось создать week_battle");
cJSON_AddItemToObject(week_battle, "active", cJSON_CreateBool(false));
cJSON_AddItemToObject(week_battle, "active_now", cJSON_CreateBool(false));

cJSON *start_time = cJSON_CreateObject();
cJSON_AddItemToObject(start_time, "DAY", cJSON_CreateNumber(0));
cJSON_AddItemToObject(start_time, "MONTH", cJSON_CreateNumber(0));
cJSON_AddItemToObject(start_time, "YEAR", cJSON_CreateNumber(0));
cJSON_AddItemToObject(week_battle, "start_time", start_time);

cJSON *last_session = cJSON_CreateObject();
cJSON_AddItemToObject(last_session, "DAY", cJSON_CreateNumber(0));
cJSON_AddItemToObject(last_session, "MONTH", cJSON_CreateNumber(0));
cJSON_AddItemToObject(last_session, "YEAR", cJSON_CreateNumber(0));
cJSON_AddItemToObject(week_battle, "last_session_time", last_session);

cJSON_AddItemToObject(week_battle, "sessions_count", cJSON_CreateNumber(0));
cJSON_AddItemToObject(week_battle, "total_damage_dealt", cJSON_CreateNumber(0));
cJSON_AddItemToObject(week_battle, "total_damage_taken", cJSON_CreateNumber(0));
cJSON_AddItemToObject(root, "week_battle", week_battle);

// Моргот
cJSON *morgoth = cJSON_CreateObject();
CHECK_NULL_ERR1(morgoth, "Не удалось создать morgoth");
cJSON_AddItemToObject(morgoth, "hp", cJSON_CreateNumber(500));
cJSON_AddItemToObject(morgoth, "max_hp", cJSON_CreateNumber(500));
cJSON_AddItemToObject(morgoth, "level", cJSON_CreateNumber(1));
cJSON_AddItemToObject(morgoth, "status", cJSON_CreateString("active"));

cJSON *retreat = cJSON_CreateObject();
cJSON_AddItemToObject(retreat, "DAY", cJSON_CreateNumber(0));
cJSON_AddItemToObject(retreat, "MONTH", cJSON_CreateNumber(0));
cJSON_AddItemToObject(retreat, "YEAR", cJSON_CreateNumber(0));
cJSON_AddItemToObject(morgoth, "retreat_until", retreat);
cJSON_AddItemToObject(root, "morgoth", morgoth);

// Дебаффы
cJSON *debuffs = cJSON_CreateObject();
CHECK_NULL_ERR1(debuffs, "Не удалось создать debuffs");
cJSON_AddItemToObject(debuffs, "damage_reduction_percent", cJSON_CreateNumber(0));
cJSON_AddItemToObject(debuffs, "morgoth_heal_amount", cJSON_CreateNumber(0));
cJSON_AddItemToObject(debuffs, "morgoth_damage_bonus_percent", cJSON_CreateNumber(0));
cJSON_AddItemToObject(root, "debuffs", debuffs);
	

	return 1;
}
	
// создание или проверка наличия Якоролевства
cJSON* check_create_ikingdom(){

	// сначала ищем файл, если нет, то создаем с нуля
	cJSON *root = file_in_json("ikingdom.json");
	if(root == NULL){
		root = cJSON_CreateObject();
		CHECK_NULL(root, "Не создается структура root\n");
		if(create_ikingdom(root) < 0){
			printf("Не удалось добавить поля для ikingdom");
			return NULL;
		}
		save_in_file(root, "ikingdom.json");

		return root;
	}

	// если есть: 
	// ПРОВЕРИТЬ ВРЕМЯ.
	
	// проверяем наличие ХП королевства
	cJSON *check_hp = cJSON_GetObjectItem(root, "health_point");
	if(check_hp == NULL){
		// создаем хп
		cJSON *health_point = cJSON_CreateNumber(150);
		CHECK_NULL(health_point, "не удалось создать объект health_point\n");
		cJSON_AddItemToObject(root, "health_point", health_point);
		save_in_file(root, "ikingdom.json");
	}
	
	// проверяем наличие скиллов
	cJSON *check_skills = cJSON_GetObjectItem(root, "skills");

	// нет скиллов, добавляем
	if(check_skills == NULL){
		// общая вкладка скиллов
		cJSON *skills = cJSON_CreateObject();
		CHECK_NULL(skills, "не удалось создать объект skills\n");

		// вкладка стандартных скиллов
		cJSON *standart_skills = cJSON_CreateObject();
		if(standart_skills == NULL){
			printf("не удалось создать объект standart_skills\n");
			cJSON_Delete(skills);
			return NULL;
		}
		// добавление стандартных скиллов
		if(add_standart_elements(standart_skills) < 0){
			printf("не удалось добавить объект В standart_skills с помощью add_standart_elements()\n");
			cJSON_Delete(skills);
			return NULL;
		}
		cJSON_AddItemToObject(skills, "standart_skills", standart_skills);
	
		// вкладка добавочных скиллов
		cJSON *my_skills = cJSON_CreateObject();
		if(my_skills == NULL){
			printf("не удалось создать объект my_skills\n");
			cJSON_Delete(skills);
			return NULL;
		}
		cJSON_AddItemToObject(skills, "my_skills", my_skills);
		
		cJSON_AddItemToObject(root, "skills", skills);

		// сохранили в файлик
		save_in_file(root, "ikingdom.json");
	}
	
	// проверка наличии кузни
	cJSON *check_forge = cJSON_GetObjectItem(root, "forge");

	// если нет, добавляем
	if(check_forge == NULL){
		// создаем и добавляем поле
		cJSON *forge = cJSON_CreateObject();
		CHECK_NULL(forge, "не удалось создать объект forge\n");
		if(add_time(forge) < 0){
			printf("Не удалось создать время\n");
			return NULL;
		}
		cJSON_AddItemToObject(root, "forge", forge);
		save_in_file(root, "ikingdom.json");
	}

	cJSON *check_table = cJSON_GetObjectItem(root, "hunter_table");
	if(check_table == NULL){
		// создаем и добавляем поле
		cJSON *hunter_table = cJSON_CreateObject();
		CHECK_NULL(hunter_table, "не удалось создать объект hunter_table\n");
		if(add_time(hunter_table) < 0){
			printf("Не удалось создать время\n");
			return NULL;
		}
		cJSON_AddItemToObject(root, "hunter_table", hunter_table);
		save_in_file(root, "ikingdom.json");
	}
	return root;
}

int main(int argc, char *argv[]){
	// инициализация
	cJSON *root = check_create_ikingdom();

	// обработчики для Якоролевство
	// время
	if(argc == 5 && strcmp(argv[1], "set_day") == 0){
		Time time; 
		time.Year = atoi(argv[2]);
		printf("%d\n", time.Year);
		time.Month = atoi(argv[3]);
		printf("%d\n", time.Month);
		time.Day = atoi(argv[4]);
		printf("%d\n", time.Day);
		if(set_day(root, time) < 0){
			printf("Не удалось поставить время по какой то ошибке\n");
			cJSON_Delete(root);
			return 0;
		}
		save_in_file(root, "ikingdom.json");
	}
	// установка своих скиллов
	else if(argc == 3 && strcmp(argv[1], "add_skill") == 0){
		if(set_skills(root, argv[2]) < 0){
			printf("Не удалось добавить скилл по какой то ошибке\n");
			cJSON_Delete(root);
			return 0;
		}
		save_in_file(root, "ikingdom.json");
	}
	// прокачка скиллов (команда имя_скилла колво_очков)
	else if(argc == 4 && strcmp(argv[1], "up_skill") == 0){
		char *names[] = {"focus", "discipline", "structure", "resilience"};
		int size = sizeof(names) / sizeof(names[0]);
		int count = atoi(argv[3]);
		for(int i = 0; i < size; i++){
			if(strcmp(argv[2], names[i]) == 0){
				if(up_standart_skill(root, argv[2], count) < 0){
					printf("Не удалось прокачать стандартный скилл\n");
					cJSON_Delete(root);
					return 0;
				}
				save_in_file(root, "ikingdom.json");
				cJSON_Delete(root);
				return 0;
			}
		}
		if(up_skills(root, argv[2], count) < 0){
			printf("Не удалось прокачать скилл\n");
			cJSON_Delete(root);
			return 0;
		}
		save_in_file(root, "ikingdom.json");
		cJSON_Delete(root);
		return 0;
	}
	// добавление жертв в столе контрактов
	else if(argc == 4 && strcmp(argv[1], "add_purpose") == 0){
		if(set_purpose(root, argv[2], argv[3]) < 0){
			printf("Не удалось поставить цель по какой то ошибке\n");
			cJSON_Delete(root);
			return 0;
		}	
	}
	// добавление достижений в кузню
	else if(argc == 3 && strcmp(argv[1], "forge") == 0){
		int count = atoi(argv[2]);
		if(add_big_ach(root, count) < 0){
			printf("Не удалось добавить большое достижение в кузню\n");
			cJSON_Delete(root);
			return 0;
		}
		save_in_file(root, "ikingdom.json");
		cJSON_Delete(root);
		return 0;
	}
	// Обработчики для библиотеки 
	else if(argc == 2 && strcmp(argv[1], "init_library") == 0){
		cJSON *library = create_library();
		if(library == NULL){
			cJSON_Delete(root);
			return 0;
		}
		printf("Библиотека создана\n");
		cJSON_Delete(root);
		cJSON_Delete(library);
		return 0;
	}
	// добавление книги
	// название, имя, колво страниц
	else if(argc == 5 && strcmp(argv[1], "add_book") == 0){
		cJSON *library = create_library();
		if(library == NULL){
			cJSON_Delete(root);
			return 0;
		}
		int pages = atoi(argv[4]);
		if(add_book(library, argv[2], argv[3], pages) < 0){
			printf("Не удалось добавить книгу\n");
			cJSON_Delete(root);
			cJSON_Delete(library);
			return 0;
		}

		printf("Книга %s добавлена\n", argv[2]);
		cJSON_Delete(root);
		cJSON_Delete(library);
		return 0;
	}
	// прочитывание книги
	// название и колво страниц
	else if(argc == 4 && strcmp(argv[1], "read_book") == 0){
		cJSON *library = create_library();
		if(library == NULL){
			cJSON_Delete(root);
			return 0;
		}
		int read_pages = atoi(argv[3]);
		if(reading_book(root, library, argv[2], read_pages) < 0){
			cJSON_Delete(root);		
			cJSON_Delete(library);
			return 0;
		}
	} // Заброшенная книга
	else if(argc == 3 && strcmp(argv[1], "abandon_book") == 0) {
		  cJSON *library = create_library();
			if (library == NULL) { cJSON_Delete(root); return 0; }
			abandon_book(library, argv[2]);
			cJSON_Delete(root);
			cJSON_Delete(library);
			return 0;
	}
	// battle start
	else if(argc == 2 && strcmp(argv[1], "battle") == 0) {
    battle_start(root);
    save_in_file(root, "ikingdom.json");
	}
	// battle attack <type> <hours>
	else if(argc == 5 && strcmp(argv[1], "battle") == 0 && strcmp(argv[2], "attack") == 0) {
    int hours = atoi(argv[4]);
    battle_attack(root, argv[3], hours);
    save_in_file(root, "ikingdom.json");
	}
	// battle morgoth <type> <hours>
	else if(argc == 5 && strcmp(argv[1], "battle") == 0 && strcmp(argv[2], "morgoth") == 0) {
    int hours = atoi(argv[4]);
    battle_morgoth_attack(root, argv[3], hours);
    save_in_file(root, "ikingdom.json");
	}
	// battle end
	else if(argc == 3 && strcmp(argv[1], "battle") == 0 && strcmp(argv[2], "end") == 0) {
    battle_end(root);
	}
	else {
		printf("Вы ввели неправильную команду\n");
	}

	cJSON_Delete(root);
	return 0;
}


