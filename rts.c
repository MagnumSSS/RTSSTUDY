#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"

#define ERR1 -1

// структура для времени
typedef struct{
	int Year;
	int Month;
	int Day;
} Time;


void save_in_file(cJSON *root, char *name);
cJSON* file_in_json(char *name);
int add_time(cJSON *root);

// загрузка из файла в json
cJSON* file_in_json(char *name){
	FILE *fp = fopen(name, "r");
	if(fp == NULL){
		printf("Не открывается файл json(1.1)\n");
		return NULL;
	}

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
	if(fp == NULL){
		printf("Не открывается файл json(1)\n");
		return;
	}

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

// добавление полей для стихий
int add_field_elements(cJSON *object){
	if(object == NULL){
		printf("Не удалось принять структуру json в add_field_elements\n");
		return ERR1;
	}

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
	if(skills == NULL){
		printf("Не удалось принять структуру json в add_standart_elements\n");
		return ERR1;
	}

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
	// добавление времени
	cJSON *time = cJSON_CreateObject();
	if(time == NULL){
		printf("не удалось создать объект time\n");
		return ERR1;
	}
	cJSON_AddItemToObject(root, "time", time);

	cJSON_AddItemToObject(time, "DAY", cJSON_CreateNumber(0));
	cJSON_AddItemToObject(time, "MONTH", cJSON_CreateNumber(0));
	cJSON_AddItemToObject(time, "YEAR", cJSON_CreateNumber(0));
	return 1;
}

// установка времени. дают структуру в которой есть структура time, достаем поля
int set_day(cJSON *root, Time time){
	if(root == NULL){
		printf("Не удалось поставить время в set_day\n");
		return ERR1;
	}
	
	cJSON *obj_time = cJSON_GetObjectItem(root, "time");
	if(obj_time == NULL){
		printf("Не удалось достать объект time в set_day\n");
		return ERR1;
	}

	cJSON *obj_day = cJSON_GetObjectItem(obj_time, "DAY");
	if(obj_day == NULL && !(cJSON_IsNumber(obj_day))){
		printf("Не удалось вытащить DAY\n");
		return ERR1;
	}
	cJSON_SetNumberValue(obj_day, time.Day);

	//printf("time.Day: %d\n", time.Day);
	//printf("DAY: %d\n", obj_day->valueint);

	cJSON *obj_month = cJSON_GetObjectItem(obj_time, "MONTH");
	if(obj_month == NULL && !(cJSON_IsNumber(obj_month))){
		printf("Не удалось вытащить MONTH\n");
		return ERR1;
	}
	cJSON_SetNumberValue(obj_month, time.Month);

	//printf("time.Month: %d\n", time.Month);
	//printf("MONTH: %d\n", obj_month->valueint);

	cJSON *obj_year = cJSON_GetObjectItem(obj_time, "YEAR");
	if(obj_year == NULL && !(cJSON_IsNumber(obj_year))){
		printf("Не удалось вытащить YEAR\n");
		return ERR1;
	}
	cJSON_SetNumberValue(obj_year, time.Year);
	
	//printf("time.Year: %d\n", time.Year);
	//printf("YEAR: %d\n", obj_year->valueint);

	return 1;
}


// создание своих скиллов
int set_skills(cJSON *root, char *name){
	if(root == NULL){
		printf("Не удалось вытащить структуру root в set_skills\n");
		return ERR1;
	}
	else if(name == NULL){
		printf("Не удалось вытащить имя скилла в set_skills\n");
		return ERR1;
	}
	
	cJSON *skills = cJSON_GetObjectItem(root, "skills");
	if(skills == NULL){
		printf("Не удалось вытащить skills в set_skills\n");
		return ERR1;
	}

	cJSON *my_skills = cJSON_GetObjectItem(skills, "my_skills");
	if(my_skills == NULL){
		printf("Не удалось вытащить my_skills в set_skills\n");
		return ERR1;
	}

	cJSON *new_skills = cJSON_CreateObject();
	if(new_skills == NULL){
		printf("Не удалось создать new_skills в set_skills\n");
		return ERR1;
	}

	if(add_field_elements(new_skills) < 0){
		printf("Не удалось добавить поля объекту new_skills в set_skills\n");
		return ERR1;
	}

	cJSON_AddItemToObject(my_skills, name, new_skills);
	return 1;
}

// подгонка времени с главной структуры в активные 
int fit_time_root(cJSON *root, cJSON *object){
	if(root == NULL){
		printf("Не удается достать root в fit_time_root\n");
		return ERR1;
	}
	else if(object == NULL){
		printf("Не удается достать object в fit_time_root\n");
		return ERR1;
	}

	// достаем структуру time из root'a 
	cJSON *time = cJSON_GetObjectItem(root, "time");
	if(time == NULL){
		printf("Не удается достать time в fit_time_root\n");
		return ERR1;
	}
	
	cJSON *day = cJSON_GetObjectItem(time, "DAY");
	if(day == NULL){
		printf("Не удается достать day в fit_time_root\n");
		return ERR1;
	}
	int iday = day->valueint;


	cJSON *month = cJSON_GetObjectItem(time, "MONTH");
	if(month == NULL){
		printf("Не удается достать month в fit_time_root\n");
		return ERR1;
	}
	int imonth = month->valueint;

	
	cJSON *year = cJSON_GetObjectItem(time, "YEAR");
	if(year == NULL){
		printf("Не удается достать year в fit_time_root\n");
		return ERR1;
	}
	int iyear = year->valueint;


	// достаем структуру time из объекта
	cJSON *obj_time = cJSON_GetObjectItem(object, "time");
	if(obj_time == NULL){
		printf("Не удается достать obj_time в fit_time_root\n");
		return ERR1;
	}

	cJSON *obj_day = cJSON_GetObjectItem(obj_time, "DAY");
	if(obj_day == NULL){
		printf("Не удается достать obj_day в fit_time_root\n");
		return ERR1;
	}
	if(obj_day->valueint != iday) cJSON_SetNumberValue(obj_day, iday);

	cJSON *obj_month = cJSON_GetObjectItem(obj_time, "MONTH");
	if(obj_month == NULL){
		printf("Не удается достать obj_month в fit_time_root\n");
		return ERR1;
	}
	if(obj_month->valueint != imonth) cJSON_SetNumberValue(obj_month, imonth);
	
	cJSON *obj_year = cJSON_GetObjectItem(obj_time, "YEAR");
	if(obj_year == NULL){
		printf("Не удается достать obj_year в fit_time_root\n");
		return ERR1;
	}
	if(obj_year->valueint != iyear) cJSON_SetNumberValue(obj_year, iyear);
	return 1;
		
}

// математическая функция прокачки скилла
int math_up(cJSON *skill, int count){
	if(skill == NULL){
		printf("Не удается достать skill в math_up\n");
		return ERR1;
	}
	
	cJSON *xp = cJSON_GetObjectItem(skill, "xp");
	if(xp == NULL && !(cJSON_IsNumber(xp))){
		printf("Не удается достать xp в math_up\n");
		return ERR1;
	}
	int ixp = xp->valueint;

	cJSON *level = cJSON_GetObjectItem(skill, "level");
	if(level == NULL && !(cJSON_IsNumber(level))){
		printf("Не удается достать level в math_up\n");
		return ERR1;
	}
	int ilevel = level->valueint;

	cJSON *thrhold_next = cJSON_GetObjectItem(skill, "threshold_next");
	if(thrhold_next == NULL && !(cJSON_IsNumber(thrhold_next))){
		printf("Не удается достать threshold_next в math_up\n");
		return ERR1;
	}
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
	if(root == NULL){
		printf("Не удалось вытащить структуру root в up_standart_skill\n");
		return ERR1; 
	}
	else if(name == NULL){
		printf("Не удалось вытащить имя скилла в up_standart_skill\n");
		return ERR1;
	}
	else if(count <= 0){
		printf("Колво очков прокачки скилла не может быть равно 0\n");
		return ERR1;
	}

	cJSON *skills = cJSON_GetObjectItem(root, "skills");
	if(skills == NULL){
		printf("Не удалось вытащить skills в up_standart_skill\n");
		return ERR1;
	}

	cJSON *standart_skills = cJSON_GetObjectItem(skills, "standart_skills");
	if(skills == NULL){
		printf("Не удалось вытащить standart_skills в up_standart_skill\n");
		return ERR1;
	}
	
	cJSON *up_skill = cJSON_GetObjectItem(standart_skills, name);
	if(up_skill == NULL){
		printf("Не удалось вытащить %s в up_standart_skill\n", name);
		return ERR1;
	}
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
	if(root == NULL){
		printf("Не удалось вытащить структуру root в up_skill\n");
		return ERR1; 
	}
	else if(name == NULL){
		printf("Не удалось вытащить имя скилла в up_skill\n");
		return ERR1;
	}
	else if(count <= 0){
		printf("Колво очков прокачки скилла не может быть равно 0\n");
		return ERR1;
	}
	
	cJSON *skills = cJSON_GetObjectItem(root, "skills");
	if(skills == NULL){
		printf("Не удалось вытащить skills в up_skill\n");
		return ERR1;
	}

	cJSON *my_skills = cJSON_GetObjectItem(skills, "my_skills");
	if(my_skills == NULL){
		printf("Не удалось вытащить my_skills в up_skill\n");
		return ERR1;
	}
	cJSON *up_skill = cJSON_GetObjectItem(my_skills, name);
	if(up_skill == NULL){
		printf("Не удалось вытащить %s в up_skill\n", name);
		return ERR1;
	}
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
	if(root == NULL){
		printf("Не удалось достать структуру root в add_big_ach\n");
		return ERR1;
	}
	else if(count <= 0){
		printf("Достижений не может быть 0\n");
		return ERR1;
	}
	
	cJSON *forge = cJSON_GetObjectItem(root, "forge");
	if(forge == NULL){
		printf("Не удалось достать структуру forge в add_big_ach\n");
		return ERR1;
	}
	
	cJSON *big_achievements = cJSON_GetObjectItem(forge, "big_achievements");
	if(big_achievements == NULL && !(cJSON_IsNumber(big_achievements))){
		printf("Не удалось достать big_achievements\n");	
		return ERR1;
	}
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
		if(library == NULL){
			printf("Не удалось создать библиотеку\n");
			return NULL;
		}
		save_in_file(library, "library.json");
		return library;
	}

	return library;
}

// добавление книги в библиотеку
int add_book(cJSON *l_root, char *book_name, char *author, int pages){
	// проверка на нулл поинтеры
	if(l_root == NULL){
		printf("Библиотека не найдена в add_book\n");
		return ERR1;
	}
	else if(book_name == NULL){
		printf("Не прочитано название книги\n");
		return ERR1;
	}
	else if(author == NULL){
		printf("Не прочитано имя автора\n");
		return ERR1;
	}
	else if(pages <= 0){
		printf("у книги не может быть 0 страниц\n");
		return ERR1;
	}

	// создание книги - структуры
	cJSON *book_obj = cJSON_CreateObject();
	if(book_obj == NULL){
		printf("Не удалось создать объект книгу\n");
		return ERR1;
	}
	

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
	if(l_root == NULL){
		printf("Не прочитан l_root в reading_book\n");
		return ERR1;
	}
	else if(name == NULL){
		printf("Не прочитано имя книги в reading_book\n");
		return ERR1;
	}
	else if(count_pages <= 0){
		printf("Ты не можешь !прочитать! 0 страниц\n");
		return ERR1;
	}
	
	// достаем книгу
	cJSON *book_obj = cJSON_GetObjectItem(l_root, name);
	if(book_obj == NULL){
		printf("Не удалось найти такую книгу\n");
		return ERR1;
	}

	// достаем поле колво страниц, колво прочитанных и статус
	cJSON *pages_obj = cJSON_GetObjectItem(book_obj, "pages");
	if(pages_obj == NULL && !(cJSON_IsNumber(pages_obj))){
		printf("Не удалось достать объект pages\n");
		return ERR1;
	}

	cJSON *rdpages_obj = cJSON_GetObjectItem(book_obj, "reading_pages");
	if(rdpages_obj == NULL && !(cJSON_IsNumber(rdpages_obj))){
		printf("Не удалось достать объект reading_pages\n");
		return ERR1;
	}

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

// добавление с нуля, возвращает -1 если плохо все
int create_ikingdom(cJSON *root){
	if(root == NULL){
		printf("Не удалось принять структуру json в create_ikingdom\n");
		return ERR1;
	}

	if(add_time(root) < 0){
		printf("Не удалось создать время\n");
		return ERR1;
	}
	
	// добавление хп
	cJSON *health_point = cJSON_CreateNumber(150);
	if(health_point == NULL){
		printf("не удалось создать объект health_point\n");
		return ERR1;
	}
	cJSON_AddItemToObject(root, "health_point", health_point);

	// общая вкладка скиллов
	cJSON *skills = cJSON_CreateObject();
	if(skills == NULL){
		printf("не удалось создать объект skills\n");
		return ERR1;
	}

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
			

	// создаем и добавляем поле
	cJSON *forge = cJSON_CreateObject();
	if(forge == NULL){
		printf("не удалось создать объект forge\n");
		return ERR1;
	}
	cJSON_AddItemToObject(root, "forge", forge);

	cJSON *forge_count = cJSON_CreateNumber(0);
	if(forge_count == NULL){
		printf("Не удалось создать объект *forge_count\n");
		return ERR1;
	}
	cJSON_AddItemToObject(forge, "big_achievements", forge_count);
	if(add_time(forge) < 0){
		printf("Не удалось добавить время кузнице\n");
		return ERR1;
	}
	
	return 1;
}
	
// создание или проверка наличия Якоролевства
cJSON* check_create_ikingdom(){

	// сначала ищем файл, если нет, то создаем с нуля
	cJSON *root = file_in_json("ikingdom.json");
	if(root == NULL){
		root = cJSON_CreateObject();
		if(root == NULL){
			printf("Не создается структура root");
			return NULL;
		}
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
		if(health_point == NULL){
			printf("не удалось создать объект health_point\n");
			return NULL; 
		}
		cJSON_AddItemToObject(root, "health_point", health_point);
		save_in_file(root, "ikingdom.json");
	}
	
	// проверяем наличие скиллов
	cJSON *check_skills = cJSON_GetObjectItem(root, "skills");

	// нет скиллов, добавляем
	if(check_skills == NULL){
		// общая вкладка скиллов
		cJSON *skills = cJSON_CreateObject();
		if(skills == NULL){
			printf("не удалось создать объект skills\n");
			return NULL;
		}

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

		return root;
	}
	
	// проверка наличии кузни
	cJSON *check_forge = cJSON_GetObjectItem(root, "forge");

	// если нет, добавляем
	if(check_forge == NULL){
		// создаем и добавляем поле
		cJSON *forge = cJSON_CreateObject();
		if(forge == NULL){
			printf("не удалось создать объект forge\n");
			return NULL;
		}
		if(add_time(forge) < 0){
			printf("Не удалось создать время\n");
			return NULL;
		}
		cJSON_AddItemToObject(root, "forge", forge);
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
			printf("Не удалось поставить время по какой то ошибке\n");
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
	}

	else {
		printf("Вы ввели неправильную команду\n");
	}

	cJSON_Delete(root);
	return 0;
}


