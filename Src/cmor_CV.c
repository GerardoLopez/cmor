#define _XOPEN_SOURCE

#define _GNU_SOURCE
#include <string.h>
#include <time.h>
#include <regex.h>
#include "cmor.h"
#include "json.h"
#include "json_tokener.h"
#include "arraylist.h"
#include "libgen.h"
/************************************************************************/
/*                        cmor_CV_set_att()                             */
/************************************************************************/
void cmor_CV_set_att(cmor_CV_def_t *CV,
                        char *szKey,
                        json_object *joValue) {
    json_bool bValue;
    double dValue;
    int nValue;
    array_list *pArray;
    int k, length;


    strcpy(CV->key, szKey);

    if( json_object_is_type( joValue, json_type_null) ) {
        printf("Will not save NULL JSON type from CV.json\n");

    } else if(json_object_is_type( joValue, json_type_boolean)) {
        bValue = json_object_get_boolean(joValue);
        CV->nValue = (int) bValue;
        CV->type = CV_integer;

    } else if(json_object_is_type( joValue, json_type_double)) {
        dValue = json_object_get_double(joValue);
        CV->dValue = dValue;
        CV->type = CV_double;

    } else if(json_object_is_type( joValue, json_type_int)) {
        nValue = json_object_get_int(joValue);
        CV->nValue = nValue;
        CV->type = CV_integer;
/* -------------------------------------------------------------------- */
/* if value is a JSON object, recursively call in this function         */
/* -------------------------------------------------------------------- */
   } else if(json_object_is_type( joValue, json_type_object)) {

        int nCVId = -1;
        int nbObject = 0;
        cmor_CV_def_t * oValue;
        int nTableID = CV->table_id; /* Save Table ID */

        json_object_object_foreach(joValue, key, value) {
            nbObject++;
            oValue = (cmor_CV_def_t *) realloc(CV->oValue,
                   sizeof(cmor_CV_def_t) * nbObject);
            CV->oValue = oValue;

            nCVId++;
            cmor_CV_init(&CV->oValue[nCVId], nTableID);
            cmor_CV_set_att(&CV->oValue[nCVId], key, value  );
        }
        CV->nbObjects=nbObject;
        CV->type = CV_object;


    } else if(json_object_is_type( joValue, json_type_array)) {
        pArray = json_object_get_array(joValue);
        length = array_list_length(pArray);
        json_object *joItem;
        CV->anElements = length;
        for(k=0; k<length; k++) {
            joItem = (json_object *) array_list_get_idx(pArray, k);
            strcpy(CV->aszValue[k], json_object_get_string(joItem));
        }
        CV->type=CV_stringarray;

    } else if(json_object_is_type( joValue, json_type_string)) {
        strcpy(CV->szValue, json_object_get_string(joValue));
        CV->type=CV_string;
    }



}


/************************************************************************/
/*                           cmor_CV_init()                             */
/************************************************************************/
void cmor_CV_init( cmor_CV_def_t *CV, int table_id ) {
    int i;

    cmor_is_setup(  );
    cmor_add_traceback("_init_CV_def");
    CV->table_id = table_id;
    CV->type = CV_undef;  //undefined
    CV->nbObjects = -1;
    CV->nValue = -1;
    CV->key[0] = '\0';
    CV->szValue[0] = '\0';
    CV->dValue=-9999.9;
    CV->oValue = NULL;
    for(i=0; i< CMOR_MAX_JSON_ARRAY; i++) {
        CV->aszValue[i][0]='\0';
    }
    CV->anElements = -1;

    cmor_pop_traceback();
}

/************************************************************************/
/*                         cmor_CV_print()                              */
/************************************************************************/
void cmor_CV_print(cmor_CV_def_t *CV) {
    int k;

    if (CV != NULL) {
        if(CV->key[0] != '\0'){
            printf("key: %s \n", CV->key);
        }
        else {
            return;
        }
        switch(CV->type) {

        case CV_string:
            printf("value: %s\n", CV->szValue);
            break;

        case CV_integer:
            printf("value: %d\n", CV->nValue);
            break;

        case CV_stringarray:
            printf("value: [\n");
            for (k = 0; k < CV->anElements; k++) {
                printf("value: %s\n", CV->aszValue[k]);
            }
            printf("        ]\n");
            break;

        case CV_double:
            printf("value: %lf\n", CV->dValue);
            break;

        case CV_object:
            printf("*** nbObjects=%d\n", CV->nbObjects);
            for(k=0; k< CV->nbObjects; k++){
                cmor_CV_print(&CV->oValue[k]);
            }
            break;

        case CV_undef:
            break;
        }

    }
}

/************************************************************************/
/*                       cmor_CV_printall()                             */
/************************************************************************/
void cmor_CV_printall() {
    int j,i;
    cmor_CV_def_t *CV;
    int nCVs;
/* -------------------------------------------------------------------- */
/* Parse tree and print key values.                                     */
/* -------------------------------------------------------------------- */
    for( i = 0; i < CMOR_MAX_TABLES; i++ ) {
        if(cmor_tables[i].CV != NULL) {
            printf("table %s\n", cmor_tables[i].szTable_id);
            nCVs = cmor_tables[i].CV->nbObjects;
            for( j=0; j<= nCVs; j++ ) {
                CV = &cmor_tables[i].CV[j];
                cmor_CV_print(CV);
            }
        }
    }
}

/************************************************************************/
/*                  cmor_CV_search_child_key()                          */
/************************************************************************/
cmor_CV_def_t *cmor_CV_search_child_key(cmor_CV_def_t *CV, char *key){
    int i;
    cmor_CV_def_t *searchCV;
    int nbCVs = -1;
    cmor_add_traceback("_CV_search_child_key");

    nbCVs = CV->nbObjects;

    // Look at this objects
    if(strcmp(CV->key, key)== 0){
        cmor_pop_traceback();
        return(CV);
    }

    // Look at each of object key
     for(i = 0; i< nbCVs; i++) {
         // Is there a branch on that object?
         if(&CV->oValue[i] != NULL) {
             searchCV = cmor_CV_search_child_key(&CV->oValue[i], key);
             if(searchCV != NULL ){
                 cmor_pop_traceback();
                 return(searchCV);
             }
         }
     }
     cmor_pop_traceback();
     return(NULL);
}

/************************************************************************/
/*                     cmor_CV_rootsearch()                             */
/************************************************************************/
cmor_CV_def_t * cmor_CV_rootsearch(cmor_CV_def_t *CV, char *key){
    int i;
    int nbCVs = -1;
    cmor_add_traceback("_CV_rootsearch");

    // Look at first objects
    if(strcmp(CV->key, key)== 0){
        cmor_pop_traceback();
        return(CV);
    }
    // Is there more than 1 object?
    if( CV->nbObjects != -1 ) {
        nbCVs = CV->nbObjects;
    }
    // Look at each of object key
    for(i = 1; i< nbCVs; i++) {
        if(strcmp(CV[i].key, key)== 0){
            cmor_pop_traceback();
            return(&CV[i]);
        }
    }
    cmor_pop_traceback();
    return(NULL);
}
/************************************************************************/
/*                      cmor_CV_get_value()                             */
/************************************************************************/
char *cmor_CV_get_value(cmor_CV_def_t *CV, char *key) {

        switch(CV->type) {
        case CV_string:
            return(CV->szValue);
            break;

        case CV_integer:
            sprintf(CV->szValue, "%d", CV->nValue);
            break;

        case CV_stringarray:
            return(CV->aszValue[0]);
            break;

        case CV_double:
            sprintf(CV->szValue, "%lf", CV->dValue);
            break;

        case CV_object:
            return(NULL);
            break;

        case CV_undef:
            CV->szValue[0]='\0';
            break;
        }

        return(CV->szValue);
}
/************************************************************************/
/*                         cmor_CV_free()                               */
/************************************************************************/
void cmor_CV_free(cmor_CV_def_t *CV) {
    int k;
/* -------------------------------------------------------------------- */
/* Recursively go down the tree and free branch                         */
/* -------------------------------------------------------------------- */
    if(CV->oValue != NULL) {
        for(k=0; k< CV->nbObjects; k++){
            cmor_CV_free(&CV->oValue[k]);
        }
    }
    if(CV->oValue != NULL) {
        free(CV->oValue);
        CV->oValue=NULL;
    }

}
/************************************************************************/
/*                    cmor_CV_checkFurtherInfoURL()                     */
/************************************************************************/
int cmor_CV_checkFurtherInfoURL(int var_id){
    char szFurtherInfoURLTemplate[CMOR_MAX_STRING];
    char szFurtherInfoURL[CMOR_MAX_STRING];
    char copyURL[CMOR_MAX_STRING];
    char szFurtherInfoBaseURL[CMOR_MAX_STRING];
    char szFurtherInfoFileURL[CMOR_MAX_STRING];
    char szValue[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];

    char *baseURL;
    char *fileURL;
    char *szToken;

    szFurtherInfoURL[0]='\0';
    szFurtherInfoFileURL[0]='\0';
    szFurtherInfoBaseURL[0]='\0';
    cmor_add_traceback("_CV_checkFurtherInfoURL");

/* -------------------------------------------------------------------- */
/* Retrieve default Further URL info                                    */
/* -------------------------------------------------------------------- */
    strncpy(szFurtherInfoURLTemplate, cmor_current_dataset.furtherinfourl,
            CMOR_MAX_STRING);

/* -------------------------------------------------------------------- */
/*    If this is a string with no token we have nothing to do.          */
/* -------------------------------------------------------------------- */
    szToken = strtok(szFurtherInfoURLTemplate, "<>");
    if( strcmp(szToken, cmor_current_dataset.furtherinfourl) == 0) {
        return(0);
    }
    strncpy(szFurtherInfoURLTemplate, cmor_current_dataset.furtherinfourl,
            CMOR_MAX_STRING);
/* -------------------------------------------------------------------- */
/*    Separate path template from file template.                        */
/* -------------------------------------------------------------------- */
    strcpy(copyURL, szFurtherInfoURLTemplate);
    baseURL = dirname(copyURL);
    cmor_CreateFromTemplate( var_id, baseURL,
            szFurtherInfoBaseURL, "/" );

    strcpy(copyURL, szFurtherInfoURLTemplate);
    fileURL = basename(copyURL);

    cmor_CreateFromTemplate( var_id, fileURL,
                             szFurtherInfoFileURL, "." );

    strncpy(szFurtherInfoURL, szFurtherInfoBaseURL, CMOR_MAX_STRING);
    strcat(szFurtherInfoURL, "/");
    strncat(szFurtherInfoURL, szFurtherInfoFileURL, strlen(szFurtherInfoFileURL));

    if(cmor_has_cur_dataset_attribute(GLOBAL_ATT_FURTHERINFOURL) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_FURTHERINFOURL, szValue );
        if( strncmp(szFurtherInfoURL, szValue, CMOR_MAX_STRING) != 0) {
            cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
            snprintf( msg, CMOR_MAX_STRING,
                    "The further info in attribute does not match "
                    "the one found in your Control Vocabulary(CV) File. \n! "
                    "We found \"%s\" and \n! "
                    "CV requires \"%s\" \n! "

                    "Check your Control Vocabulary file \"%s\".\n! ",
                    szValue,
                    szFurtherInfoURL,
                    CV_Filename);

            cmor_handle_error( msg, CMOR_WARNING );
            cmor_pop_traceback(  );
            return(-1);

        }

    }
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_FURTHERINFOURL,
            szFurtherInfoURL, 0);
    cmor_pop_traceback( );
    return(0);
}

/************************************************************************/
/*                            get_CV_Error()                            */
/************************************************************************/
int get_CV_Error(){
    return(CV_ERROR);
}


/************************************************************************/
/*                      cmor_CV_checkSourceType()                       */
/************************************************************************/
int cmor_CV_checkSourceType(cmor_CV_def_t *CV_exp, char *szExptID){
    int nObjects;
    cmor_CV_def_t *CV_exp_attr;
    char szAddSourceType[CMOR_MAX_STRING];
    char szReqSourceType[CMOR_MAX_STRING];
    char szAddSourceTypeCpy[CMOR_MAX_STRING];
    char szReqSourceTypeCpy[CMOR_MAX_STRING];

    char szSourceType[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    int i, j;
    char *szTokenRequired;
    char *szTokenAdd;
    int nbSourceType;
    char *ptr;
    int nbGoodType=0;
    cmor_add_traceback("_CV_checkSourceType");

    szAddSourceType[0] = '\0';
    szReqSourceType[0] = '\0';
    szAddSourceTypeCpy[0] = '\0';
    szReqSourceTypeCpy[0] = '\0';
    szSourceType[0] = '\0';

    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);

    szAddSourceType[0] = '\0';
    nObjects = CV_exp->nbObjects;
    // Parse all experiment attributes get source type related attr.
    for (i = 0; i < nObjects; i++) {
        CV_exp_attr = &CV_exp->oValue[i];
        if(strcmp(CV_exp_attr->key, CV_EXP_ATTR_ADDSOURCETYPE) == 0 ) {
        	for(j = 0; j< CV_exp_attr->anElements; j++) {
        		strcat(szAddSourceType, CV_exp_attr->aszValue[j]);
        		strcat(szAddSourceType, " ");
        		strcat(szAddSourceTypeCpy, CV_exp_attr->aszValue[j]);
        		strcat(szAddSourceTypeCpy, " ");

        	}
            continue;
        }
        if(strcmp(CV_exp_attr->key, CV_EXP_ATTR_REQSOURCETYPE) == 0) {
        	for(j = 0; j< CV_exp_attr->anElements; j++) {
        		strcat(szReqSourceType, CV_exp_attr->aszValue[j]);
        		strcat(szReqSourceType, " ");
        		strcat(szReqSourceTypeCpy, CV_exp_attr->aszValue[j]);
        		strcat(szReqSourceTypeCpy, " ");

        	}
            continue;
        }

    }
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_SOURCE_TYPE) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_SOURCE_TYPE, szSourceType);

        // Count how many are token  we have.
        ptr = szSourceType;
        if (szSourceType[0] != '\0') {
            nbSourceType = 1;
        } else {
            cmor_pop_traceback();
            return(-1);
        }
        while ((ptr = strpbrk(ptr, " ")) != NULL) {
            nbSourceType++;
            ptr++;
        }
    }

    szTokenRequired = strtok(szReqSourceType, " ");

    while(szTokenRequired != NULL) {
        if(strstr(szSourceType, szTokenRequired) == NULL) {

            snprintf(msg, CMOR_MAX_STRING,
                    "The following source type(s) \"%s\" are required and\n! "
                            "some source type(s) could not be found in your "
                            "input file. \n! "
                            "Your file contains a source type of \"%s\".\n! "
                            "Check your Control Vocabulary file \"%s\".\n! ",
                    szReqSourceTypeCpy, szSourceType, CV_Filename);

            cmor_handle_error(msg, CMOR_NORMAL);
            cmor_pop_traceback();
            return(-1);
        } else {
            nbGoodType++;
        }
        szTokenRequired = strtok(NULL, " ");
    }

    szTokenAdd = strtok(szAddSourceType, " ");
    while (szTokenAdd != NULL) {
        if (strcmp(szTokenAdd, "CHEM") == 0) {
            if (strstr(szSourceType, szTokenAdd) != NULL) {
                nbGoodType++;
            }
        } else if (strcmp(szTokenAdd, "AER") == 0) {
            if (strstr(szSourceType, szTokenAdd) != NULL) {
                nbGoodType++;
            }
        } else if (strstr(szSourceType, szTokenAdd) != NULL) {
            nbGoodType++;
        }
        szTokenAdd = strtok(NULL, " ");
    }

    if (nbGoodType != nbSourceType) {
        snprintf(msg, CMOR_MAX_STRING,
                "You source_type attribute contains invalid source types\n! "
                        "Your source type is set to \"%s\".  The required source types\n! "
                        "are \"%s\" and possible additional source types are \"%s\" \n! "
                        "Check your Control Vocabulary file \"%s\".\n! %d, %d",
                szSourceType, szReqSourceTypeCpy, szAddSourceTypeCpy,
                CV_Filename, nbGoodType, nbSourceType);
        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return(-1);
    }

    cmor_pop_traceback(  );
    return(0);


}
/************************************************************************/
/*                      cmor_CV_checkSourceID()                         */
/************************************************************************/
int cmor_CV_checkSourceID(cmor_CV_def_t *CV){
    cmor_CV_def_t *CV_source_ids;
    cmor_CV_def_t *CV_source_id = NULL;

    char szSource_ID[CMOR_MAX_STRING];
    char szSource[CMOR_MAX_STRING];

    char msg[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    int rc;
    int i,j;

    cmor_is_setup(  );
    cmor_add_traceback("_CV_checkSourceID");
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
/* -------------------------------------------------------------------- */
/*  Find experiment_ids dictionary in Control Vocabulary                */
/* -------------------------------------------------------------------- */
    CV_source_ids = cmor_CV_rootsearch(CV, CV_KEY_SOURCE_IDS);

    if(CV_source_ids == NULL){
        snprintf( msg, CMOR_MAX_STRING,
                "Your \"source_ids\" key could not be found in\n! "
                "your Control Vocabulary file.(%s)\n! ",
                CV_Filename);

        cmor_handle_error( msg, CMOR_NORMAL );
        cmor_pop_traceback(  );
        return(-1);
    }


    // retrieve source_id
    rc = cmor_get_cur_dataset_attribute(GLOBAL_ATT_SOURCE_ID, szSource_ID);
    if( rc !=  0 ){
        snprintf( msg, CMOR_MAX_STRING,
                "You \"%s\" is not defined, check your required attributes\n! "
                "See Control Vocabulary JSON file.(%s)\n! ",
                GLOBAL_ATT_SOURCE_ID,
                CV_Filename);
        cmor_handle_error( msg, CMOR_NORMAL );
        cmor_pop_traceback(  );
        return(-1);
    }

    // Validate  source_id
    for (i = 0; i < CV_source_ids->nbObjects; i++) {
        CV_source_id = &CV_source_ids->oValue[i];
        if (strncmp(CV_source_id->key, szSource_ID, CMOR_MAX_STRING) == 0) {
            // Make sure that "source" exist.
            if(cmor_has_cur_dataset_attribute(GLOBAL_ATT_SOURCE) != 0 ) {
                cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_SOURCE,
                        CV_source_id->aszValue[0], 1);
            }
            // Check source with experiment_id label.
            rc = cmor_get_cur_dataset_attribute(GLOBAL_ATT_SOURCE, szSource);
            for( j=0; j < CV_source_id->nbObjects; j++) {
            	if( strcmp(CV_source_id->oValue[j].key, CV_KEY_SOURCE_LABEL) == 0) {
            		break;
            	}
            }
            if( j == CV_source_id->nbObjects) {
                snprintf(msg, CMOR_MAX_STRING,
                        "Could not find %s string in experiment_id section.\n! \n! \n!"
                        "See Control Vocabulary JSON file. (%s)\n! ",
                                CV_KEY_SOURCE_LABEL,
                                CV_Filename);
                cmor_handle_error(msg, CMOR_WARNING);
                break;
            }
            if (strncmp(CV_source_id->oValue[j].szValue, szSource, CMOR_MAX_STRING) != 0) {
                snprintf(msg, CMOR_MAX_STRING,
                        "Your input attribute \"%s\" with value \n! \"%s\" "
                                "will be replaced with "
                                "value \n! \"%s\".\n! \n! \n!  "
                                "See Control Vocabulary JSON file.(%s)\n! ",
                                GLOBAL_ATT_SOURCE, szSource,
                                CV_source_id->oValue[j].szValue,
                                CV_Filename);
                cmor_handle_error(msg, CMOR_WARNING);
            }
            break;
        }
    }
    // We could not found the Source ID in the CV file
    if( i == CV_source_ids->nbObjects) {


        snprintf( msg, CMOR_MAX_STRING,
                "The source_id, \"%s\",  which you specified in your \n! "
                "input file could not be found in \n! "
                "your Controlled Vocabulary file. (%s) \n! \n! "
                "Please correct your input file or contact "
                "cmor@listserv.llnl.gov to register\n! "
                "a new source.   ",
                szSource_ID, CV_Filename);

        cmor_handle_error(msg, CMOR_NORMAL);
        return(-1);
    }
    // Set/replace attribute.
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_SOURCE_ID,
            CV_source_id->key,1);
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_SOURCE,
            CV_source_id->oValue[j].szValue,1);

    cmor_pop_traceback();
    return(0);
}

/************************************************************************/
/*                     cmor_CV_checkExperiment()                        */
/************************************************************************/
int cmor_CV_checkExperiment( cmor_CV_def_t *CV){
    cmor_CV_def_t *CV_experiment_ids;
    cmor_CV_def_t *CV_experiment;
    cmor_CV_def_t *CV_experiment_attr;

    char szExperiment_ID[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    char szValue[CMOR_MAX_STRING];
    char szExpValue[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    int rc;
    int nObjects;
    int i;
    int j;
    int bWarning;

    szExpValue[0] = '\0';
    cmor_add_traceback("_CV_checkExperiment");
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
    cmor_get_cur_dataset_attribute(GLOBAL_ATT_EXPERIMENTID, szExperiment_ID);
/* -------------------------------------------------------------------- */
/*  Find experiment_ids dictionary in Control Vocabulary                */
/* -------------------------------------------------------------------- */
    CV_experiment_ids = cmor_CV_rootsearch(CV, CV_KEY_EXPERIMENT_IDS);
    if(CV_experiment_ids == NULL){
        snprintf( msg, CMOR_MAX_STRING,
                "Your \"experiment_ids\" key could not be found in\n! "
                "your Control Vocabulary file.(%s)\n! ",
                CV_Filename);

        cmor_handle_error( msg, CMOR_NORMAL );
        cmor_pop_traceback(  );
        return(-1);
    }
    CV_experiment = cmor_CV_search_child_key( CV_experiment_ids,
                                               szExperiment_ID);

    if(CV_experiment == NULL){
        snprintf( msg, CMOR_MAX_STRING,
                "Your experiment_id \"%s\" defined in your input file\n! "
                "could not be found in your Control Vocabulary file.(%s)\n! ",
                szExperiment_ID,
                CV_Filename);

        cmor_handle_error( msg, CMOR_NORMAL );
        cmor_pop_traceback(  );
        return(-1);
    }

    nObjects = CV_experiment->nbObjects;
    // Parse all experiment attributes
    for (i = 0; i < nObjects; i++) {
        bWarning = FALSE;
        CV_experiment_attr = &CV_experiment->oValue[i];
        rc = cmor_has_cur_dataset_attribute(CV_experiment_attr->key);
        // Validate source type first
        if(strcmp(CV_experiment_attr->key, CV_EXP_ATTR_REQSOURCETYPE) == 0) {
            cmor_CV_checkSourceType(CV_experiment, szExperiment_ID);
            continue;
        }
        // Warn user if experiment value from input file is different than
        // Control Vocabulary value.
        // experiment from Control Vocabulary will replace User entry value.
        if( rc == 0) {
            cmor_get_cur_dataset_attribute(CV_experiment_attr->key, szValue);
            if( CV_experiment_attr->anElements > 0) {
            	for( j = 0; j < CV_experiment_attr->anElements; j++) {
            		//
            		// Find a string that match this value in the list?
            		//
            		 if( strncmp( CV_experiment_attr->aszValue[j], szValue,
            				      CMOR_MAX_STRING) == 0) {
            			 break;
            		 }
            	}
				if (j == CV_experiment_attr->anElements) {
					strcpy(szExpValue, CV_experiment_attr->aszValue[0] );
					bWarning = TRUE;
				}
			} else
        		//
        		// Check for string instead of list of string object!
        		//
				if( CV_experiment_attr->szValue[0] != '\0') {
					if (strncmp(CV_experiment_attr->szValue, szValue,
							CMOR_MAX_STRING) != 0) {
						strcpy(szExpValue,CV_experiment_attr->szValue);
						bWarning = TRUE;
					}
				}
        }
		if (bWarning == TRUE) {
			snprintf(msg, CMOR_MAX_STRING,
					"Your input attribute \"%s\" with value \n! \"%s\" "
							"will be replaced with "
							"value \"%s\"\n! "
							"as defined for experiment_id \"%s\".\n! \n!  "
							"See Control Vocabulary JSON file.(%s)\n! ",
					CV_experiment_attr->key, szValue,
					szExpValue, CV_experiment->key,
					CV_Filename);
			cmor_handle_error(msg, CMOR_WARNING);
		}
		// Set/replace attribute.
		cmor_set_cur_dataset_attribute_internal(CV_experiment_attr->key,
				CV_experiment_attr->szValue, 1);
	}
	cmor_pop_traceback();

	return(0);
}

/************************************************************************/
/*                      cmor_CV_setInstitution()                        */
/************************************************************************/
int cmor_CV_setInstitution( cmor_CV_def_t *CV){
    cmor_CV_def_t *CV_institution_ids;
    cmor_CV_def_t *CV_institution;

    char szInstitution_ID[CMOR_MAX_STRING];
    char szInstitution[CMOR_MAX_STRING];

    char msg[CMOR_MAX_STRING];
    char CMOR_Filename[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    int rc;

    cmor_add_traceback("_CV_setInstitution");
/* -------------------------------------------------------------------- */
/*  Find current Insitution ID                                          */
/* -------------------------------------------------------------------- */

    cmor_get_cur_dataset_attribute(GLOBAL_ATT_INSTITUTION_ID, szInstitution_ID);
    rc = cmor_has_cur_dataset_attribute(CMOR_INPUTFILENAME);
    if (rc == 0) {
        cmor_get_cur_dataset_attribute(CMOR_INPUTFILENAME, CMOR_Filename);
    }
    else {
        CMOR_Filename[0] = '\0';
    }
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);

/* -------------------------------------------------------------------- */
/*  Find Institution dictionaries in Control Vocabulary                 */
/* -------------------------------------------------------------------- */
    CV_institution_ids = cmor_CV_rootsearch(CV, CV_KEY_INSTITUTION_IDS);
    if(CV_institution_ids == NULL) {
        snprintf( msg, CMOR_MAX_STRING,
                "Your \"institution_ids\" key could not be found in\n! "
                "your Control Vocabulary file.(%s)\n! ",
                CV_Filename);

        cmor_handle_error( msg, CMOR_NORMAL );
        cmor_pop_traceback(  );
        return(-1);
    }
    CV_institution = cmor_CV_search_child_key( CV_institution_ids,
                                               szInstitution_ID);

    if(CV_institution == NULL){
        snprintf( msg, CMOR_MAX_STRING,
                "The institution_id, \"%s\",  found in your \n! "
                "input file (%s) could not be found in \n! "
                "your Controlled Vocabulary file. (%s) \n! \n! "
                "Please correct your input file or contact "
                "\"cmor@listserv.llnl.gov\" to register\n! "
                "a new institution_id.  \n! \n! "
                "See \"http://cmor.llnl.gov/mydoc_cmor3_CV/\" for further information about\n! "
                "the \"institution_id\" and \"institution\" global attributes.  " ,
                szInstitution_ID, CMOR_Filename, CV_Filename);
        cmor_handle_error( msg, CMOR_NORMAL );
        cmor_pop_traceback(  );
        return(-1);
    }
/* -------------------------------------------------------------------- */
/* Did the user defined an Institution Attribute?                       */
/* -------------------------------------------------------------------- */

    rc = cmor_has_cur_dataset_attribute( GLOBAL_ATT_INSTITUTION );
    if( rc == 0 ) {
/* -------------------------------------------------------------------- */
/* Retrieve institution and compare with the one we have in the         */
/* Control Vocabulary file.  If it does not matches tell the user       */
/* the we will supersede his definition with the one in the Control     */
/* Vocabulary file.                                                     */
/* -------------------------------------------------------------------- */
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_INSTITUTION, szInstitution);

/* -------------------------------------------------------------------- */
/*  Check if an institution has been defined! If not we exit.           */
/* -------------------------------------------------------------------- */
        if(CV_institution->szValue[0] == '\0'){
            snprintf( msg, CMOR_MAX_STRING,
                    "There is no institution associated to institution_id \"%s\"\n! "
                    "in your Control Vocabulary file.\n! "
                    "Check your institution_ids dictionary!!\n! ",
                            szInstitution_ID);
            cmor_handle_error( msg, CMOR_NORMAL );
            cmor_pop_traceback(  );
            return(-1);
        }
/* -------------------------------------------------------------------- */
/*  Check if they have the same string                                  */
/* -------------------------------------------------------------------- */
        if(strncmp(szInstitution, CV_institution->szValue, CMOR_MAX_STRING) != 0){
            snprintf( msg, CMOR_MAX_STRING,
                    "Your input attribute institution \"%s\" will be replaced with \n! "
                    "\"%s\" as defined in your Control Vocabulary file.\n! ",
                    szInstitution, CV_institution->szValue);
            cmor_handle_error( msg, CMOR_WARNING );
        }
    }
/* -------------------------------------------------------------------- */
/*   Set institution according to the Control Vocabulary                */
/* -------------------------------------------------------------------- */
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_INSTITUTION,
                                            CV_institution->szValue, 1);
    cmor_pop_traceback();
    return(0);
}

/************************************************************************/
/*                    cmor_CV_ValidateAttribute()                       */
/*                                                                      */
/*  Search for attribute and verify that values is within a list        */
/*                                                                      */
/*    i.e. "mip_era": [ "CMIP6", "CMIP5" ]                              */
/************************************************************************/
int cmor_CV_ValidateAttribute(cmor_CV_def_t *CV, char *szKey){
    cmor_CV_def_t *attr_CV;
    char szValue[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    char szValids[CMOR_MAX_STRING*2];
    char szOutput[CMOR_MAX_STRING*2];
    int i;
    regex_t regex;
    int reti;
    int ierr;

    cmor_add_traceback( "_CV_ValidateAttribute" );

    szValids[0] = '\0';
    szOutput[0] = '\0';
    attr_CV = cmor_CV_rootsearch(CV, szKey);
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);

/* -------------------------------------------------------------------- */
/* This attribute does not need validation.                             */
/* -------------------------------------------------------------------- */
    if(attr_CV == NULL) {
        cmor_pop_traceback(  );
        return(0);
    }

    ierr = cmor_get_cur_dataset_attribute( szKey, szValue );
    for(i=0; i< attr_CV->anElements; i++) {
        reti = regcomp(&regex, attr_CV->aszValue[i],0);
        if(reti) {
            snprintf( msg, CMOR_MAX_STRING,
                    "You regular expression \"%s\" is invalid. \n!"
                    "Check your Control Vocabulary file \"%s\".\n! ",
                    attr_CV->aszValue[i], CV_Filename );
            regfree(&regex);
            cmor_handle_error( msg, CMOR_NORMAL );
            cmor_pop_traceback(  );
            return(-1);

        }
/* -------------------------------------------------------------------- */
/*        Execute regular expression                                    */
/* -------------------------------------------------------------------- */
        reti = regexec(&regex, szValue, 0, NULL, 0);
        if(!reti) {
            regfree(&regex);
            break;
        }
        regfree(&regex);
    }

    if( ierr != 0) {
        cmor_pop_traceback(  );
        return(-1);
    }
/* -------------------------------------------------------------------- */
/* We could not validate this attribute, exit.                          */
/* -------------------------------------------------------------------- */
    if( i == (attr_CV->anElements) ) {
        for(i=0; i< attr_CV->anElements; i++) {
            strcat(szValids, "\"");
            strncat(szValids, attr_CV->aszValue[i], CMOR_MAX_STRING);
            strcat(szValids, "\" ");
        }
        snprintf( szOutput, 132, "%s ...", szValids);

        snprintf( msg, CMOR_MAX_STRING,
                  "The attribute \"%s\" could not be validated. \n! "
                  "The current input value is "
                  "\"%s\" which is not valid \n! "
                  "Valid values must match the regular expression:"
                  "\n! \t[%s] \n! \n! "
                  "Check your Control Vocabulary file \"%s\".\n! ",
                  szKey, szValue, szOutput, CV_Filename);

        cmor_handle_error( msg, CMOR_NORMAL );
        cmor_pop_traceback(  );
        return(-1);
    }
    cmor_pop_traceback(  );
    return(0);
}

/************************************************************************/
/*                        cmor_CV_checkGrids()                          */
/************************************************************************/
int  cmor_CV_checkGrids(cmor_CV_def_t *CV) {
    int rc;
    char szGridLabel[CMOR_MAX_STRING];
    char szGridResolution[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];

    cmor_CV_def_t *CV_grid_labels;
    cmor_CV_def_t *CV_grid_resolution;
    int i;

    cmor_add_traceback( "_CV_checkGrids" );

    rc = cmor_has_cur_dataset_attribute( GLOBAL_ATT_GRID_LABEL );
    if( rc == 0 ) {
        cmor_get_cur_dataset_attribute( GLOBAL_ATT_GRID_LABEL,
                                        szGridLabel);
    }
/* -------------------------------------------------------------------- */
/*  "gr followed by a digit is a valid grid (regrid)                    */
/* -------------------------------------------------------------------- */
    if( ( strcmp(szGridLabel, CV_KEY_GRIDLABEL_GR ) >= '0') &&
        ( strcmp(szGridLabel, CV_KEY_GRIDLABEL_GR ) <= '9')) {
        strcpy(szGridLabel, CV_KEY_GRIDLABEL_GR);
    }

    rc = cmor_has_cur_dataset_attribute( GLOBAL_ATT_GRID_RESOLUTION );
    if( rc == 0 ) {
        cmor_get_cur_dataset_attribute( GLOBAL_ATT_GRID_RESOLUTION,
                                        szGridResolution);
    }

    CV_grid_labels = cmor_CV_rootsearch(CV, CV_KEY_GRID_LABELS);
    if(CV_grid_labels == NULL) {
        snprintf( msg, CMOR_MAX_STRING,
                "Your \"grid_labels\" key could not be found in\n! "
                "your Control Vocabulary file.(%s)\n! ",
                CV_Filename);

        cmor_handle_error( msg, CMOR_NORMAL );
        cmor_pop_traceback(  );
        return(-1);
    }
    if (CV_grid_labels->anElements > 0) {
    		for (i = 0; i < CV_grid_labels->anElements; i++) {
    			rc = strcmp(CV_grid_labels->aszValue[i], szGridLabel);
    			if (rc == 0) {
    				break;
    			}
    		}
    		if (i == CV_grid_labels->anElements) {
    			snprintf(msg, CMOR_MAX_STRING,
    					"Your attribute grid_label is set to \"%s\" which is invalid."
    					"\n! \n! Check your Control Vocabulary file \"%s\".\n! ",
    					szGridLabel, CV_Filename);
    			cmor_handle_error(msg, CMOR_NORMAL);
    			cmor_pop_traceback();
    			return(-1);

    		}
    }
    CV_grid_resolution = cmor_CV_rootsearch(CV, CV_KEY_GRID_RESOLUTION);
    if(CV_grid_resolution == NULL ) {
        snprintf( msg, CMOR_MAX_STRING,
                  "Your attribute grid_label is set to \"%s\" which is invalid."
                  "\n! \n! Check your Control Vocabulary file \"%s\".\n! ",
                  szGridLabel, CV_Filename);
        cmor_handle_error( msg, CMOR_NORMAL );
        cmor_pop_traceback();
        return(-1);

    }

	if (CV_grid_resolution->anElements > 0) {
		for (i = 0; i < CV_grid_resolution->anElements; i++) {
			rc = strcmp(CV_grid_resolution->aszValue[i], szGridResolution);
			if (rc == 0) {
				break;
			}
		}
		if (i == CV_grid_resolution->anElements) {
			snprintf(msg, CMOR_MAX_STRING,
					"Your attribute grid_resolution is set to \"%s\" which is invalid."
							"\n! \n! Check your Control Vocabulary file \"%s\".\n! ",
					szGridResolution, CV_Filename);
			cmor_handle_error(msg, CMOR_NORMAL);
			cmor_pop_traceback();
			return(-1);

		}
	}

    cmor_pop_traceback(  );
    return(0);
}

/************************************************************************/
/*                    cmor_CV_CheckGblAttributes()                      */
/************************************************************************/
int cmor_CV_checkGblAttributes( cmor_CV_def_t *CV ) {
    cmor_CV_def_t *required_attrs;
    int i;
    int rc;
    char msg[CMOR_MAX_STRING];
    int bCriticalError = FALSE;
    int ierr =0;
    cmor_add_traceback( "_CV_checkGblAttributes" );
    required_attrs = cmor_CV_rootsearch(CV, CV_KEY_REQUIRED_GBL_ATTRS);
    if( required_attrs != NULL) {
        for(i=0; i< required_attrs->anElements; i++) {
            rc = cmor_has_cur_dataset_attribute( required_attrs->aszValue[i] );
            if( rc != 0) {
                snprintf( msg, CMOR_MAX_STRING,
                          "Your Control Vocabulary file specifies one or more\n! "
                          "required attributes.  The following\n! "
                          "attribute was not properly set.\n! \n! "
                          "Please set attribute: \"%s\" in your input file.",
                          required_attrs->aszValue[i] );
                cmor_handle_error( msg, CMOR_NORMAL );
                bCriticalError = TRUE;
                ierr +=-1;
            }
            rc = cmor_CV_ValidateAttribute(CV, required_attrs->aszValue[i]);
            if( rc != 0) {
                bCriticalError = TRUE;
                ierr +=-1;
            }
        }
    }
    if( bCriticalError ) {
        cmor_handle_error( "Please fix required attributes mentioned in\n! "
                           "the warnings/error above and rerun. (aborting!)\n! "
                           ,CMOR_NORMAL );
    }
    cmor_pop_traceback(  );
    return(ierr);
}


/************************************************************************/
/*                       cmor_CV_set_entry()                            */
/************************************************************************/
int cmor_CV_set_entry(cmor_table_t* table,
                      json_object *value) {
    extern int cmor_ntables;
    int nCVId;
    int nbObjects = 0;
    cmor_CV_def_t *CV;
    cmor_CV_def_t *newCV;
    cmor_table_t *cmor_table;
    cmor_table = &cmor_tables[cmor_ntables];

    cmor_is_setup();

    cmor_add_traceback("_CV_set_entry");
/* -------------------------------------------------------------------- */
/* CV 0 contains number of objects                                      */
/* -------------------------------------------------------------------- */
    nbObjects++;
    newCV = (cmor_CV_def_t *) realloc(cmor_table->CV, sizeof(cmor_CV_def_t));

    cmor_table->CV = newCV;
    CV=newCV;
    cmor_CV_init(CV, cmor_ntables);
    cmor_table->CV->nbObjects=nbObjects;

/* -------------------------------------------------------------------- */
/*  Add all values and dictionaries to the M-tree                       */
/*                                                                      */
/*  {  { "a":[] }, {"a":""}, { "a":1 }, "a":"3", "b":"value" }....      */
/*                                                                      */
/*   Cv->objects->objects->list                                         */
/*              ->objects->string                                       */
/*              ->objects->integer                                      */
/*              ->integer                                               */
/*              ->string                                                */
/*     ->list                                                           */
/* -------------------------------------------------------------------- */
    json_object_object_foreach(value, CVName, CVValue) {
        nbObjects++;
        newCV = (cmor_CV_def_t *) realloc(cmor_table->CV,
                nbObjects * sizeof(cmor_CV_def_t));
        cmor_table->CV = newCV;
        nCVId = cmor_table->CV->nbObjects;

        CV = &cmor_table->CV[nCVId];

        cmor_CV_init(CV, cmor_ntables);
        cmor_table->CV->nbObjects++;

        if( CVName[0] == '#') {
            continue;
        }
        cmor_CV_set_att(CV, CVName, CVValue);
    }
    CV = &cmor_table->CV[0];
    CV->nbObjects = nbObjects;
    cmor_pop_traceback();
    return (0);
}


/************************************************************************/
/*                       cmor_CV_checkTime()                            */
/************************************************************************/
int cmor_CV_checkISOTime(char *szAttribute) {
    struct tm tm;
    int rc;
    char *szReturn;
    char szDate[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];

    rc = cmor_has_cur_dataset_attribute(szAttribute );
    if( rc == 0 ) {
/* -------------------------------------------------------------------- */
/*   Retrieve Date                                                      */
/* -------------------------------------------------------------------- */
        cmor_get_cur_dataset_attribute(szAttribute, szDate);
    }
/* -------------------------------------------------------------------- */
/*    Check data format                                                 */
/* -------------------------------------------------------------------- */
    memset(&tm, 0, sizeof(struct tm));
    szReturn=strptime(szDate, "%FT%H:%M:%SZ", &tm);
    if(szReturn == NULL) {
            snprintf( msg, CMOR_MAX_STRING,
                      "Your global attribute "
                      "\"%s\" set to \"%s\" is not a valid date.\n! "
                      "ISO 8601 date format \"YYYY-MM-DDTHH:MM:SSZ\" is required."
                      "\n! ", szAttribute, szDate);
            cmor_handle_error( msg, CMOR_NORMAL );
            cmor_pop_traceback(  );
            return(-1);
    }
    cmor_pop_traceback(  );
    return(0);
}



/************************************************************************/
/*                         cmor_CV_variable()                           */
/************************************************************************/
int cmor_CV_variable( int *var_id, char *name, char *units, float *missing ) {

    int vrid=-1;
    int i;
    int iref;
    char msg[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    cmor_var_def_t refvar;

    cmor_is_setup(  );

    cmor_add_traceback( "cmor_CV_variable" );

    if( CMOR_TABLE == -1 ) {
        cmor_handle_error( "You did not define a table yet!",
                           CMOR_CRITICAL );
    }

/* -------------------------------------------------------------------- */
/*      ok now look which variable is corresponding in table if not     */
/*      found then error                                                */
/* -------------------------------------------------------------------- */
    iref = -1;
    cmor_trim_string( name, ctmp );
    for( i = 0; i < cmor_tables[CMOR_TABLE].nvars + 1; i++ ) {
        if( strcmp( cmor_tables[CMOR_TABLE].vars[i].id, ctmp ) == 0 ) {
            iref = i;
            break;
        }
    }
/* -------------------------------------------------------------------- */
/*      ok now look for out_name to see if we can it.                   */
/* -------------------------------------------------------------------- */

    if (iref == -1) {
        for (i = 0; i < cmor_tables[CMOR_TABLE].nvars + 1; i++) {
            if (strcmp(cmor_tables[CMOR_TABLE].vars[i].out_name, ctmp) == 0) {
                iref = i;
                break;
            }
        }
    }

    if( iref == -1 ) {
        snprintf( msg, CMOR_MAX_STRING,
                  "Could not find a matching variable for name: '%s'",
                  ctmp );
        cmor_handle_error( msg, CMOR_CRITICAL );
    }

    refvar = cmor_tables[CMOR_TABLE].vars[iref];
    for( i = 0; i < CMOR_MAX_VARIABLES; i++ ) {
        if( cmor_vars[i].self == -1 ) {
            vrid = i;
            break;
        }
    }


    cmor_vars[vrid].ref_table_id = CMOR_TABLE;
    cmor_vars[vrid].ref_var_id = iref;

/* -------------------------------------------------------------------- */
/*      init some things                                                */
/* -------------------------------------------------------------------- */

    strcpy( cmor_vars[vrid].suffix, "" );
    strcpy( cmor_vars[vrid].base_path, "" );
    strcpy( cmor_vars[vrid].current_path, "" );
    cmor_vars[vrid].suffix_has_date = 0;

/* -------------------------------------------------------------------- */
/*      output missing value                                            */
/* -------------------------------------------------------------------- */

    cmor_vars[vrid].omissing =
        ( double ) cmor_tables[CMOR_TABLE].missing_value;

/* -------------------------------------------------------------------- */
/*      copying over values from ref var                                */
/* -------------------------------------------------------------------- */

    cmor_vars[vrid].valid_min = refvar.valid_min;
    cmor_vars[vrid].valid_max = refvar.valid_max;
    cmor_vars[vrid].ok_min_mean_abs = refvar.ok_min_mean_abs;
    cmor_vars[vrid].ok_max_mean_abs = refvar.ok_max_mean_abs;
    cmor_vars[vrid].shuffle = refvar.shuffle;
    cmor_vars[vrid].deflate = refvar.deflate;
    cmor_vars[vrid].deflate_level = refvar.deflate_level;

    if (refvar.out_name[0] == '\0') {
        strncpy(cmor_vars[vrid].id, name, CMOR_MAX_STRING);
    } else {
        strncpy(cmor_vars[vrid].id, refvar.out_name, CMOR_MAX_STRING);
    }

    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_STANDARDNAME, 'c',
            refvar.standard_name);

    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_LONGNAME, 'c',
            refvar.long_name);

    if ((refvar.flag_values != NULL) && (refvar.flag_values[0] != '\0')) {
        cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_FLAGVALUES, 'c',
                refvar.flag_values);
    }
    if ((refvar.flag_meanings != NULL) && (refvar.flag_meanings[0] != '\0')) {

        cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_FLAGMEANINGS,
                'c', refvar.flag_meanings);
    }

    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_COMMENT, 'c',
            refvar.comment);

    if (strcmp(refvar.units, "?") == 0) {
        strncpy(cmor_vars[vrid].ounits, units, CMOR_MAX_STRING);
    } else {
        strncpy(cmor_vars[vrid].ounits, refvar.units, CMOR_MAX_STRING);
    }

    if (refvar.type != 'c') {
        cmor_set_variable_attribute_internal(vrid,
                VARIABLE_ATT_UNITS,
                'c',
                cmor_vars[vrid].ounits);
    }

    strncpy(cmor_vars[vrid].iunits, units, CMOR_MAX_STRING);

    cmor_set_variable_attribute_internal( vrid, VARIABLE_ATT_CELLMETHODS,
            'c',
            refvar.cell_methods );

    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_CELLMEASURES, 'c',
            refvar.cell_measures);

    if (refvar.positive == 'u') {
        if (cmor_is_required_variable_attribute(refvar, VARIABLE_ATT_POSITIVE)
                == 0) {

            cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_POSITIVE,
                    'c', "up");

        }

    } else if (refvar.positive == 'd') {
        if (cmor_is_required_variable_attribute(refvar, VARIABLE_ATT_POSITIVE)
                == 0) {

            cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_POSITIVE,
                    'c', "down");

        }
    }

    if( refvar.type == '\0' ) {
        cmor_vars[vrid].type = 'f';
    }
    else {
        cmor_vars[vrid].type = refvar.type;
    }



    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_MISSINGVALUES,
                'f', missing);
    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_FILLVAL, 'f',
            missing);

    cmor_vars[vrid].self = vrid;
    *var_id = vrid;
    cmor_pop_traceback(  );
    return( 0 );
};
