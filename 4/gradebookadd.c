#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <sqlite3.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <ctype.h>
#include "functions.h"

int main(int argc, char *argv[])
{
    if (argc >= 7)
    {
        //Argument checking
        if (strcmp(argv[1], "-N") != 0)
        {
            printf("invalid");
            return 255;
        }
        if (!check(argv[2]))
        {
            printf("invalid");
            return 255;
        }
        if (!file_test(argv[2]))
        {
            printf("invalid");

            return 255;
        }
        if (strcmp(argv[3], "-K") != 0)
        {
            return 255;
        }
        if (datahex(argv[4]) == NULL)
        {
            printf("invalid");
            return 255;
        }

        sqlite3 *db;
        char *err_msg = 0;
        int rc;
        char *sql;
        sqlite3_stmt *stmt = 0;
        if (strcmp(argv[5], "-AA") == 0)
        {
            int an = 0;
            int p = 0;
            int w = 0;
            //More argument parsing
            for (int i = 6; i < argc - 1; i += 2)
            {
                if (strcmp(argv[i], "-AN") == 0)
                {
                    an = i + 1;
                }
                else if (strcmp(argv[i], "-P") == 0)
                {
                    p = i + 1;
                }
                else if (strcmp(argv[i], "-W") == 0)
                {
                    w = i + 1;
                }
                else
                {
                    printf("invalid");
                    return 255;
                }
            }
            //Check validity of all arguments
            if (!an || !p || !w || !checkDigit(argv[p]) || !checkFloat(argv[w]) || atoi(argv[p]) < 0 || !(atof(argv[w]) >= 0 && atof(argv[w]) <= 1) || !check(argv[an]))
            {
                printf("invalid");
                return 255;
            }

            //File decyrption
            if(decryptt(argv[2], datahex(argv[4]))){
                return 255;
            }
            rc = sqlite3_open(argv[2], &db);
            if (rc)
            {
                printf("invalid");
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));
                return 255;
            }

            //Sql query to get the sum of the assignment weights and make sure they're less than or equal to 1
            sql = "SELECT SUM(Weight) FROM Assignments;";

            rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
            
            if (rc != SQLITE_OK)
            {
                printf("invalid");
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));
                return 255;
            }
            rc = sqlite3_step(stmt);
            if (rc == SQLITE_ROW)
            {
                float a = (float)sqlite3_column_double(stmt, 0);
                if (a + atof(argv[w]) > 1)
                {
                    printf("invalid");
                    sqlite3_close(db);

                    encryptt(argv[2], datahex(argv[4]));
                    return 255;
                }
            }

            sqlite3_finalize(stmt);

            //Query to insert the assignment
            sql = "INSERT INTO Assignments VALUES(NULL, ?, ?, ?);";
            rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
            if (rc != SQLITE_OK)
            {
                printf("invalid");
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));
                return 255;
            }
            //Using prepared statement binding
            rc = sqlite3_bind_text(stmt, 1, argv[an], -1, SQLITE_STATIC);
            rc = sqlite3_bind_int(stmt, 2, (int)atoi(argv[p]));
            rc = sqlite3_bind_double(stmt, 3, (double)atof(argv[w]));

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE)
            {
                printf("invalid");
                sqlite3_finalize(stmt);
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));
                return 255;
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }
        else if (strcmp(argv[5], "-DA") == 0)
        {
            int an = 0;

            for (int i = 6; i < argc - 1; i += 2)
            {
                if (strcmp(argv[i], "-AN") == 0)
                {
                    an = i + 1;
                }

                else
                {
                    printf("invalid");
                    return 255;
                }
            }

            if (!an || !check(argv[an]))
            {
                printf("invalid");
                return 255;
            }

            if(decryptt(argv[2], datahex(argv[4]))){
                return 255;
            }

            rc = sqlite3_open(argv[2], &db);
            sql = "PRAGMA foreign_keys=ON;";
            rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

            //Query to delete an assignment by name
            sql = "DELETE FROM Assignments WHERE Name=?;";
            rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
            
            if (rc != SQLITE_OK)
            {
                printf("invalid");
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));
                return 255;
            }

            rc = sqlite3_bind_text(stmt, 1, argv[an], -1, SQLITE_STATIC);
            rc = sqlite3_step(stmt);

            while (rc == SQLITE_ROW)
            {
                rc = sqlite3_step(stmt);
            }
            if (rc != SQLITE_DONE)
            {
                printf("invalid");
                sqlite3_finalize(stmt);
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));
                return 255;
            }
            sqlite3_finalize(stmt);

            //If no deletions were made we know the assignment didn't exist
            if (sqlite3_changes(db) < 1)
            {
                printf("invalid");
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));
                return 255;
            }
            sqlite3_close(db);
        }
        else if (strcmp(argv[5], "-AS") == 0)
        {
            int fn = 0;
            int ln = 0;
            for (int i = 6; i < argc - 1; i += 2)
            {
                if (strcmp(argv[i], "-FN") == 0)
                {
                    fn = i + 1;
                }
                else if (strcmp(argv[i], "-LN") == 0)
                {
                    ln = i + 1;
                }
                else
                {
                    printf("invalid");
                    return 255;
                }
            }

            if (!fn || !ln || !checkAlpha(argv[fn]) || !checkAlpha(argv[ln]))
            {
                printf("invalid");
                return 255;
            }
            if(decryptt(argv[2], datahex(argv[4]))){
                return 255;
            }

            rc = sqlite3_open(argv[2], &db);
            //Query to add a student
            sql = "INSERT INTO Students VALUES(NULL, ?, ?);";
            rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
            if (rc != SQLITE_OK)
            {
                printf("invalid");
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));
                return 255;
            }
            rc = sqlite3_bind_text(stmt, 1, argv[fn], -1, SQLITE_STATIC);
            rc = sqlite3_bind_text(stmt, 2, argv[ln], -1, SQLITE_STATIC);

            rc = sqlite3_step(stmt);

            while (rc == SQLITE_ROW)
            {
                rc = sqlite3_step(stmt);
            }
            if (rc != SQLITE_DONE)
            {
                printf("invalid");
                sqlite3_finalize(stmt);
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));

                return 255;
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }
        else if (strcmp(argv[5], "-DS") == 0)
        {
            int fn = 0;
            int ln = 0;

            for (int i = 6; i < argc - 1; i += 2)
            {
                if (strcmp(argv[i], "-FN") == 0)
                {
                    fn = i + 1;
                }
                else if (strcmp(argv[i], "-LN") == 0)
                {
                    ln = i + 1;
                }
                else
                {
                    printf("%s", argv[i]);
                    return 255;
                }
            }
            if (!fn || !ln || !checkAlpha(argv[fn]) || !checkAlpha(argv[ln]))
            {
                printf("invalid");
                return 255;
            }
            if(decryptt(argv[2], datahex(argv[4]))){
                return 255;
            }
            rc = sqlite3_open(argv[2], &db);
            sql = "PRAGMA foreign_keys=ON;";
            rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
            //Query to delete student by name
            sql = "DELETE FROM Students WHERE FirstName=? AND LastName=?;";
            rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
            if (rc != SQLITE_OK)
            {
                printf("invalid");
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));
                return 255;
            }
            rc = sqlite3_bind_text(stmt, 1, argv[fn], -1, SQLITE_STATIC);
            rc = sqlite3_bind_text(stmt, 2, argv[ln], -1, SQLITE_STATIC);

            rc = sqlite3_step(stmt);

            while (rc == SQLITE_ROW)
            {
                rc = sqlite3_step(stmt);
            }
            if (rc != SQLITE_DONE)
            {
                printf("invalid");
                sqlite3_finalize(stmt);
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));

                return 255;
            }
            sqlite3_finalize(stmt);

            if (sqlite3_changes(db) < 1)
            {
                printf("invalid");
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));

                return 255;
            }
            sqlite3_close(db);
        }
        else if (strcmp(argv[5], "-AG") == 0)
        {

            int fn = 0;
            int ln = 0;
            int an = 0;
            int g = 0;
            for (int i = 6; i < argc - 1; i += 2)
            {
                if (strcmp(argv[i], "-FN") == 0)
                {
                    fn = i + 1;
                }
                else if (strcmp(argv[i], "-LN") == 0)
                {
                    ln = i + 1;
                }
                else if (strcmp(argv[i], "-AN") == 0)
                {
                    an = i + 1;
                }
                else if (strcmp(argv[i], "-G") == 0)
                {
                    g = i + 1;
                }
                else
                {
                    printf("invalid");
                    return 255;
                }
            }

            if (!an || !ln || !an || !checkDigit(argv[g]) || atoi(argv[g]) < 0 || !check(argv[an]) || !checkAlpha(argv[fn]) || !checkAlpha(argv[ln]))
            {
                printf("invalid");
                return 255;
            }

            if(decryptt(argv[2], datahex(argv[4]))){
                return 255;
            }
            rc = sqlite3_open(argv[2], &db);
            if (rc)
            {
                printf("invalid");
                sqlite3_close(db);
                encryptt(argv[2], datahex(argv[4]));

                return 255;
            }

            //Query to insert a grade, makes sure to update a value if one already existed
            sql = "INSERT INTO StudentScores SELECT s.StudentID,a.AssignmentID,? FROM Assignments as a INNER JOIN Students as s ON s.FirstName=? AND s.LastName=?  WHERE a.Name=? ON CONFLICT(StudentID,AssignmentID) DO UPDATE SET Points=?;";
            rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
            if (rc != SQLITE_OK)
            {
                printf("invalid");
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));
                return 255;
            }
            rc = sqlite3_bind_int(stmt, 1, (int)atoi(argv[g]));

            rc = sqlite3_bind_text(stmt, 2, argv[fn], -1, SQLITE_STATIC);
            rc = sqlite3_bind_text(stmt, 3, argv[ln], -1, SQLITE_STATIC);
            rc = sqlite3_bind_text(stmt, 4, argv[an], -1, SQLITE_STATIC);
            rc = sqlite3_bind_int(stmt, 5, (int)atoi(argv[g]));

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE)
            {
                printf("invalid");
                sqlite3_finalize(stmt);
                sqlite3_close(db);
                encryptt(argv[2], datahex(argv[4]));

                return 255;
            }
            sqlite3_finalize(stmt);
            if (sqlite3_changes(db) < 1)
            {
                printf("invalid");
                sqlite3_close(db);
                encryptt(argv[2], datahex(argv[4]));

                return 255;
            }
            sqlite3_close(db);
        }
        else
        {
            printf("invalid");
            return 255;
        }
        sqlite3_free(err_msg);

        encryptt(argv[2], datahex(argv[4]));
    }
    else
    {
        printf("invalid");
        return 255;
    }
}

