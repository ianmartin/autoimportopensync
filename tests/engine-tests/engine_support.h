#ifndef ENGINE_SUPPORT_H_
#define ENGINE_SUPPORT_H_

typedef struct OSyncDebugGroup {
	OSyncGroup *group;
	OSyncMember *member1;
	OSyncClient *client1;
	
	OSyncMember *member2;
	OSyncClient *client2;
	
	OSyncPlugin *plugin;
	OSyncPlugin *plugin2;
} OSyncDebugGroup;

#endif /*ENGINE_SUPPORT_H_*/
