#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_RECORDS 300
#define TITLE_LEN 60
#define CATEGORY_LEN 30
#define LOCATION_LEN 50
#define COLOR_LEN 25
#define DATE_LEN 16
#define OWNER_LEN 40
#define FILE_NAME "campusfind_data.txt"

typedef struct {
    int id;
    int type; /* 1 = LOST, 2 = FOUND */
    char title[TITLE_LEN];
    char category[CATEGORY_LEN];
    char color[COLOR_LEN];
    char location[LOCATION_LEN];
    char date[DATE_LEN];
    char reporter[OWNER_LEN];
    char status[16]; /* OPEN, MATCHED, RETURNED */
} Report;

Report reports[MAX_RECORDS];
int reportCount = 0;
int nextId = 1001;

/* Read a line safely and remove the trailing newline. */
void readLine(const char *prompt, char *buffer, size_t size) {
    int ch;
    printf("%s", prompt);
    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }
    if (strchr(buffer, '\n') == NULL) {
        while ((ch = getchar()) != '\n' && ch != EOF) { }
    } else {
        buffer[strcspn(buffer, "\n")] = '\0';
    }
}

/* Convert a string to lowercase in place. */
void toLowerCopy(const char *source, char *target, size_t size) {
    size_t i;
    for (i = 0; i + 1 < size && source[i] != '\0'; i++) {
        target[i] = (char)tolower((unsigned char)source[i]);
    }
    target[i] = '\0';
}

/* Case-insensitive substring check. */
int containsIgnoreCase(const char *text, const char *query) {
    char a[160], b[100];
    toLowerCopy(text, a, sizeof(a));
    toLowerCopy(query, b, sizeof(b));
    return strstr(a, b) != NULL;
}

int readInt(const char *prompt, int minValue, int maxValue) {
    char line[80], extra;
    int value;
    while (1) {
        readLine(prompt, line, sizeof(line));
        if (sscanf(line, "%d %c", &value, &extra) == 1 &&
            value >= minValue && value <= maxValue) {
            return value;
        }
        printf("Please enter a whole number from %d to %d.\n", minValue, maxValue);
    }
}

void printLine(void) {
    printf("--------------------------------------------------------------------------\n");
}

void printReport(const Report *r) {
    printf("ID: %d | %s | Status: %s\n", r->id,
           r->type == 1 ? "LOST" : "FOUND", r->status);
    printf("Item: %s | Category: %s | Color: %s\n", r->title, r->category, r->color);
    printf("Location: %s | Date: %s | Reporter: %s\n", r->location, r->date, r->reporter);
    printLine();
}

/* Text-file format: fields are separated by |. Input validation disallows |. */
int saveAll(void) {
    FILE *fp = fopen(FILE_NAME, "w");
    int i;
    if (fp == NULL) {
        printf("Error: could not open data file for writing.\n");
        return 0;
    }
    for (i = 0; i < reportCount; i++) {
        Report *r = &reports[i];
        fprintf(fp, "%d|%d|%s|%s|%s|%s|%s|%s|%s\n",
                r->id, r->type, r->title, r->category, r->color,
                r->location, r->date, r->reporter, r->status);
    }
    fclose(fp);
    return 1;
}

int hasDelimiter(const char *s) {
    return strchr(s, '|') != NULL;
}

void loadAll(void) {
    FILE *fp = fopen(FILE_NAME, "r");
    char line[400];
    if (fp == NULL) return;

    while (fgets(line, sizeof(line), fp) != NULL && reportCount < MAX_RECORDS) {
        Report r;
        int fields = sscanf(line, "%d|%d|%59[^|]|%29[^|]|%24[^|]|%49[^|]|%15[^|]|%39[^|]|%15[^\n]",
            &r.id, &r.type, r.title, r.category, r.color, r.location,
            r.date, r.reporter, r.status);
        if (fields == 9 && (r.type == 1 || r.type == 2)) {
            reports[reportCount++] = r;
            if (r.id >= nextId) nextId = r.id + 1;
        }
    }
    fclose(fp);
}

void addReport(void) {
    Report r;
    if (reportCount >= MAX_RECORDS) {
        printf("Storage is full. Cannot add more reports.\n");
        return;
    }
    memset(&r, 0, sizeof(r));
    r.id = nextId++;
    r.type = readInt("Report type (1 = Lost, 2 = Found): ", 1, 2);

    readLine("Item name (e.g., blue water bottle): ", r.title, sizeof(r.title));
    readLine("Category (e.g., electronics, books, ID card): ", r.category, sizeof(r.category));
    readLine("Color / identifying feature: ", r.color, sizeof(r.color));
    readLine("Location (where lost/found): ", r.location, sizeof(r.location));
    readLine("Date (YYYY-MM-DD): ", r.date, sizeof(r.date));
    readLine("Reporter name or nickname: ", r.reporter, sizeof(r.reporter));

    if (r.title[0] == '\0' || r.category[0] == '\0' ||
        r.location[0] == '\0' || hasDelimiter(r.title) ||
        hasDelimiter(r.category) || hasDelimiter(r.color) ||
        hasDelimiter(r.location) || hasDelimiter(r.date) ||
        hasDelimiter(r.reporter)) {
        printf("A required field was empty or contained the reserved | character. Report cancelled.\n");
        return;
    }

    strcpy(r.status, "OPEN");
    reports[reportCount++] = r;
    if (saveAll()) {
        printf("Report saved successfully. Your report ID is %d.\n", r.id);
        printf("Tip: keep the ID for status updates.\n");
    }
}

void listReports(int typeFilter, int openOnly) {
    int i, shown = 0;
    for (i = 0; i < reportCount; i++) {
        if (typeFilter != 0 && reports[i].type != typeFilter) continue;
        if (openOnly && strcmp(reports[i].status, "OPEN") != 0) continue;
        printReport(&reports[i]);
        shown++;
    }
    if (shown == 0) printf("No matching reports found.\n");
}

void searchReports(void) {
    char query[100];
    int i, shown = 0;
    readLine("Enter keyword (item, category, color, or location): ", query, sizeof(query));
    if (query[0] == '\0') {
        printf("Search keyword cannot be empty.\n");
        return;
    }
    for (i = 0; i < reportCount; i++) {
        if (containsIgnoreCase(reports[i].title, query) ||
            containsIgnoreCase(reports[i].category, query) ||
            containsIgnoreCase(reports[i].color, query) ||
            containsIgnoreCase(reports[i].location, query)) {
            printReport(&reports[i]);
            shown++;
        }
    }
    if (shown == 0) printf("No reports matched that keyword.\n");
}

/* Matching score: category 35, title keywords 30, color 20, location 15. */
int matchScore(const Report *lost, const Report *found) {
    int score = 0;
    if (containsIgnoreCase(lost->category, found->category) ||
        containsIgnoreCase(found->category, lost->category)) score += 35;
    if (containsIgnoreCase(lost->title, found->title) ||
        containsIgnoreCase(found->title, lost->title)) score += 30;
    if (lost->color[0] && found->color[0] &&
        (containsIgnoreCase(lost->color, found->color) ||
         containsIgnoreCase(found->color, lost->color))) score += 20;
    if (containsIgnoreCase(lost->location, found->location) ||
        containsIgnoreCase(found->location, lost->location)) score += 15;
    return score;
}

void suggestMatches(void) {
    int i, j, any = 0;
    printf("\nPOSSIBLE MATCHES (suggestions only; verify with the owner)\n");
    printLine();
    for (i = 0; i < reportCount; i++) {
        if (reports[i].type != 1 || strcmp(reports[i].status, "OPEN") != 0) continue;
        for (j = 0; j < reportCount; j++) {
            int score;
            if (reports[j].type != 2 || strcmp(reports[j].status, "OPEN") != 0) continue;
            score = matchScore(&reports[i], &reports[j]);
            if (score >= 35) {
                printf("Lost ID %d <-> Found ID %d | Match score: %d/100\n",
                       reports[i].id, reports[j].id, score);
                printf("Lost: %s (%s, %s) at %s\n", reports[i].title,
                       reports[i].category, reports[i].color, reports[i].location);
                printf("Found: %s (%s, %s) at %s\n",
                       reports[j].title, reports[j].category, reports[j].color,
                       reports[j].location);
                printf("Verify details manually before marking as matched.\n");
                printLine();
                any = 1;
            }
        }
    }
    if (!any) printf("No likely matches yet. Add more detailed reports and try again.\n");
}

void updateStatus(void) {
    int id = readInt("Enter report ID: ", 1, 999999);
    int i, found = 0, choice;
    for (i = 0; i < reportCount; i++) {
        if (reports[i].id == id) {
            found = 1;
            printf("Current status: %s\n", reports[i].status);
            choice = readInt("New status (1 = OPEN, 2 = MATCHED, 3 = RETURNED): ", 1, 3);
            if (choice == 1) strcpy(reports[i].status, "OPEN");
            else if (choice == 2) strcpy(reports[i].status, "MATCHED");
            else strcpy(reports[i].status, "RETURNED");
            if (saveAll()) printf("Status updated and saved.\n");
            break;
        }
    }
    if (!found) printf("No report exists with ID %d.\n", id);
}

void showStatistics(void) {
    int i, lost = 0, found = 0, open = 0, matched = 0, returned = 0;
    for (i = 0; i < reportCount; i++) {
        if (reports[i].type == 1) lost++;
        else found++;
        if (strcmp(reports[i].status, "OPEN") == 0) open++;
        else if (strcmp(reports[i].status, "MATCHED") == 0) matched++;
        else if (strcmp(reports[i].status, "RETURNED") == 0) returned++;
    }
    printLine();
    printf("CAMPUSFIND SUMMARY\n");
    printf("Total reports: %d\nLost reports: %d\nFound reports: %d\n",
           reportCount, lost, found);
    printf("Open: %d | Matched: %d | Returned: %d\n", open, matched, returned);
    printLine();
}

void showMenu(void) {
    printf("\n========== CAMPUSFIND ==========\n");
    printf("Smart Campus Lost & Found Matcher\n");
    printf("1. Add Lost/Found Report\n");
    printf("2. View All Reports\n");
    printf("3. Search Reports\n");
    printf("4. Suggest Possible Matches\n");
    printf("5. Update Report Status\n");
    printf("6. View Statistics\n");
    printf("7. Exit\n");
}

int main(void) {
    int choice;
    loadAll();
    printf("Welcome to CampusFind! Loaded %d saved report(s).\n", reportCount);
    do {
        showMenu();
        choice = readInt("Choose an option (1-7): ", 1, 7);
        switch (choice) {
            case 1: addReport(); break;
            case 2: listReports(0, 0); break;
            case 3: searchReports(); break;
            case 4: suggestMatches(); break;
            case 5: updateStatus(); break;
            case 6: showStatistics(); break;
            case 7: saveAll(); printf("Thank you for using CampusFind. Goodbye!\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 7);
    return 0;
}