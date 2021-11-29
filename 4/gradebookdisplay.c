#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sqlite3.h>
#include <ctype.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include "functions.h"

int main(int argc, char *argv[])
{

    if (argc >= 7)
    {
        //Argument parsing and checking for validity
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
        char *sql;

        int rc;
        sqlite3_stmt *stmt = 0;
        if (strcmp(argv[5], "-PA") == 0)
        {
            if (argc < 9)
            {
                printf("invalid");

                return 255;
            }
            int an = 0;
            int g = 0;
            int a = 0;
            for (int i = 6; i < argc; i++)
            {

                if (strcmp(argv[i], "-AN") == 0)
                {
                    if (i + 1 >= argc)
                    {
                        printf("invalid");
                        return 255;
                    }
                    an = i + 1;
                    i++;
                }
                else if (strcmp(argv[i], "-G") == 0)
                {
                    g = i;
                }
                else if (strcmp(argv[i], "-A") == 0)
                {
                    a = i;
                }
                else
                {
                    printf("invalid");
                    return 255;
                }
            }
            if (!an || (g && a) || (!g && !a) || !check(argv[an]))
            {
                printf("invalid");
                return 255;
            }
            if (decryptt(argv[2], datahex(argv[4])))
            {
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

            //Queries to get the assignment scores, ordering in different ways
            if (g)
            {
                sql = "SELECT b.LastName, b.FirstName, s.Points FROM Assignments AS a INNER JOIN StudentScores as s ON s.StudentID = b.studentID INNER JOIN Students as b WHERE a.Name=? AND a.AssignmentID=s.AssignmentID ORDER BY s.Points DESC;";
            }
            else if (a)
            {
                sql = "SELECT b.LastName, b.FirstName, s.Points FROM Assignments AS a INNER JOIN StudentScores as s ON s.StudentID = b.studentID INNER JOIN Students as b WHERE a.Name=? AND a.AssignmentID=s.AssignmentID ORDER BY b.LastName COLLATE NOCASE, b.FirstName COLLATE NOCASE;";
            }
            rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

            if (rc != SQLITE_OK)
            {
                printf("invalid");
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));
                return 255;
            }
            rc = sqlite3_bind_text(stmt, 1, argv[an], -1, SQLITE_STATIC);

            //Checks if there was nothing to print
            if (sqlite3_step(stmt) == SQLITE_DONE)
            {
                printf("invalid");
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));
                return 255;
            }
            else
            {
                printf("(%s, %s, %d)\n", sqlite3_column_text(stmt, 0), sqlite3_column_text(stmt, 1), sqlite3_column_int(stmt, 2));

                while (sqlite3_step(stmt) == SQLITE_ROW)
                {

                    printf("(%s, %s, %d)\n", sqlite3_column_text(stmt, 0), sqlite3_column_text(stmt, 1), sqlite3_column_int(stmt, 2));
                }
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }
        else if (strcmp(argv[5], "-PS") == 0)
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
            if (decryptt(argv[2], datahex(argv[4])))
            {
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
            //Query to print all student assignment scores
            sql = "SELECT a.Name, s.Points FROM Assignments AS a INNER JOIN StudentScores as s ON s.StudentID = b.studentID INNER JOIN Students as b WHERE b.FirstName=? AND b.LastName=? AND s.AssignmentID=a.AssignmentID;";
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

            //Checks if the student existed
            if (sqlite3_step(stmt) == SQLITE_DONE)
            {
                printf("invalid");
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));
                return 255;
            }
            else
            {
                printf("(%s, %d)\n", sqlite3_column_text(stmt, 0), sqlite3_column_int(stmt, 1));

                while (sqlite3_step(stmt) == SQLITE_ROW)
                {

                    printf("(%s, %d)\n", sqlite3_column_text(stmt, 0), sqlite3_column_int(stmt, 1));
                }
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }
        else if (strcmp(argv[5], "-PF") == 0)
        {
            int g = 0;
            int a = 0;
            for (int i = 6; i < argc; i++)
            {

                if (strcmp(argv[i], "-G") == 0)
                {
                    g = i;
                }
                else if (strcmp(argv[i], "-A") == 0)
                {
                    a = i;
                }
                else
                {
                    printf("invalid");
                    return 255;
                }
            }
            if ((g && a) || (!g && !a))
            {
                printf("invalid");
                return 255;
            }
            if (decryptt(argv[2], datahex(argv[4])))
            {
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
            //Query to get final grades, does a subquery to get the grade info per student, then orders by different methods
            if (g)
            {
                sql = "SELECT LastName, FirstName, (SELECT Sum(CAST(s.Points AS FLOAT)/a.Points*a.Weight) FROM Assignments as a INNER JOIN StudentScores as s WHERE b.StudentID=s.StudentID AND a.AssignmentID=s.AssignmentID) as x  from Students as b ORDER by x DESC;";
            }
            else if (a)
            {
                sql = "SELECT LastName, FirstName, (SELECT Sum(CAST(s.Points AS FLOAT)/a.Points*a.Weight) FROM Assignments as a INNER JOIN StudentScores as s WHERE b.StudentID=s.StudentID AND a.AssignmentID=s.AssignmentID) from Students as b ORDER by LastName COLLATE NOCASE, FirstName COLLATE NOCASE;";
            }
            rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
            if (rc != SQLITE_OK)
            {
                printf("invalid");
                sqlite3_close(db);

                encryptt(argv[2], datahex(argv[4]));
                return 255;
            }
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {

                printf("(%s, %s, %g)\n", sqlite3_column_text(stmt, 0), sqlite3_column_text(stmt, 1), sqlite3_column_double(stmt, 2));
            }

            sqlite3_finalize(stmt);
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

