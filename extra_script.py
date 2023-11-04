Import("env")

fw_name = (env["PIOENV"]) # get environment name
env.Replace(PROGNAME="%s_fw_%s" % (fw_name, env.GetProjectOption("custom_fw_version"))) # set custom firmware name & version
