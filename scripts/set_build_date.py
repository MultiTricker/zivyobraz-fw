import datetime
Import("env")

build_date = datetime.datetime.now().strftime("%Y%m%d")
env.Append(CPPDEFINES=[("BUILD_DATE", f'\\"{build_date}\\"')])
