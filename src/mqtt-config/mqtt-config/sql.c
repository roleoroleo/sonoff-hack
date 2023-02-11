#include "sql.h"

//-----------------------------------------------------------------------------
// GENERAL STATIC VARS AND FUNCTIONS
//-----------------------------------------------------------------------------

sqlite3 *dbc = NULL;

//=============================================================================

//-----------------------------------------------------------------------------
// INIT
//-----------------------------------------------------------------------------

int sql_init()
{
    int ret = 0;

    ret = sqlite3_open_v2(IPCSYS_DB, &dbc, SQLITE_OPEN_READWRITE, NULL);

    if (ret != SQLITE_OK) {
        fprintf(stderr, "Error opening db\n");
        return -1;
    }

    return 0;
}

void sql_stop()
{
    if (dbc) {
        sqlite3_close_v2(dbc);
    }

}

int sql_update(char *key, char * value)
{
    int ret;
    char buffer[1024];
    char stmp[16];

    if (strcasecmp("no", value) == 0) {
        strcpy(stmp, "0");
    } else if (strcasecmp("yes", value) == 0) {
        strcpy(stmp, "1");
    }

    if (strcasecmp("sensitivity", key) == 0) {
        if (strcasecmp("off", value) == 0) {
            sprintf (buffer, "update t_mdarea set c_sensitivity=%s where c_index=0;", "0");
        } else if (strcasecmp("low", value) == 0) {
            sprintf (buffer, "update t_mdarea set c_sensitivity=%s where c_index=0;", "25");
        } else if (strcasecmp("medium", value) == 0) {
            sprintf (buffer, "update t_mdarea set c_sensitivity=%s where c_index=0;", "50");
        } else if (strcasecmp("high", value) == 0) {
            sprintf (buffer, "update t_mdarea set c_sensitivity=%s where c_index=0;", "75");
        } else {
            sprintf (buffer, "update t_mdarea set c_sensitivity=%s where c_index=0;", value);
        }
    } else if (strcasecmp("motion_detection", key) == 0) {
        sprintf (buffer, "update t_mdarea set c_enable=%s where c_index=0;", stmp);
    } else if (strcasecmp("local_record", key) == 0) {
        sprintf (buffer, "update t_record_plan set c_enabled=%s where c_recplan_no=1;", stmp);
    } else if (strcasecmp("ir", key) == 0) {
        if (strcasecmp("auto", value) == 0) {
            sprintf (buffer, "update t_sys_param set c_param_value=%s where c_param_name=\"InfraredLamp\";", "2");
        } else if (strcasecmp("on", value) == 0) {
            sprintf (buffer, "update t_sys_param set c_param_value=%s where c_param_name=\"InfraredLamp\";", "0");
        } else if (strcasecmp("off", value) == 0) {
            sprintf (buffer, "update t_sys_param set c_param_value=%s where c_param_name=\"InfraredLamp\";", "1");
        }
    } else if (strcasecmp("rotate", key) == 0) {
        sprintf (buffer, "update t_sys_param set c_param_value=%s where c_param_name=\"flip\" or c_param_name=\"mirror\";", stmp);
    } else if (strcasecmp("switch_on", key) == 0) {
        return 0;
    } else {
        fprintf(stderr, "Unknown key %s\n", key);
        return -1;
    }

    ret = sqlite3_exec(dbc, buffer, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        fprintf(stderr, "Failed to update data: %s\n", sqlite3_errmsg(dbc));
        sqlite3_close(dbc);
        return -2;
    }

    return 0;
}
