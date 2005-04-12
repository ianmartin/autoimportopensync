import opensync_impl

class POSyncEnv(opensync_impl.OSyncEnv, object):
	def getx(self):
		return self.__x
	
	def setx(self, x):
		if x < 0: x = 0
		self.__x = x
		
	x = property(getx, setx)
