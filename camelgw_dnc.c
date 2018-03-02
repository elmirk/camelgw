


/* remove digit 7 in first digits position */
int camelgw_dnc_remove7(char *str)
{
    char *char_ptr;
		    //we should delete first 7 in DP2 MOC in Calling Party Address

    //if (str[0] == '7') {

     char_ptr = str;

	 while (*char_ptr != '\0')
	{
	    *char_ptr = *(char_ptr +1);
	    char_ptr++;

	}
	 *char_ptr = '\0';
	 //}
	 return 0;
}
