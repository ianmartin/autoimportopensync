version = "0.18"

env = Environment()

# Get our configuration options:
opts = Options('libopensync.conf')
opts.Add('prefix', 'Directory to install under', '/usr/local')
opts.Update(env)
opts.Save('libopensync.conf', env)

env.Append(CCFLAGS = '-DOPENSYNC_PLUGINDIR=\"\\"$prefix/libs/opensync/plugins\\"\"')
env.Append(CCFLAGS = '-DOPENSYNC_FORMATSDIR=\"\\"$prefix/libs/opensync/formats\\"\"')
env.Append(CCFLAGS = '-DOPENSYNC_CONFIGDIR=\"\\"$prefix/share/opensync/defaults\\"\"')
env.Append(CCFLAGS = '-DVERSION=\"\\"' + version + '\\"\"' )
env.Append(CCFLAGS = '-Werror -Wall' )
env.Append(CPPPATH = '.' )

env.ParseConfig('pkg-config --cflags --libs glib-2.0')
env.ParseConfig('pkg-config --cflags --libs libxml-2.0')

install_prefix = '$prefix'
install_lib    = '$prefix/lib'
install_bin    = '$prefix/bin'
install_inc    = '$prefix/include'

Export('env install_prefix install_lib install_bin install_inc')

SConscript(['opensync/SConscript', 'osengine/SConscript', 'formats/SConscript'])
