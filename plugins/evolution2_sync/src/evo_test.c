#include <libecal/e-cal.h>
#include <libebook/e-book.h>

int main(void)
{
	printf("test\n");


	GSList *g;
	ESourceList *cal_sources = NULL;
	if (!e_cal_get_sources(&cal_sources, E_CAL_SOURCE_TYPE_EVENT, NULL)) {
		printf("dammid\n");
	}
	
	for (g = e_source_list_peek_groups (cal_sources); g; g = g->next) {
		ESourceGroup *group = E_SOURCE_GROUP (g->data);
		GSList *s;
		for (s = e_source_group_peek_sources (group); s; s = s->next) {
			ESource *source = E_SOURCE (s->data);
			printf("asd %s\n", e_source_get_uri(source));
		}
	}

  /*if(!e_cal_open (client, FALSE, &error)) 
  {
    printf ("failed to open calendar\n");
    return 1;
  }*/
	
	return 0;
}
