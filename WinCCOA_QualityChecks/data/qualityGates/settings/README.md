# Description of the possible setting parameters

| Parameter | id                                           |
|-----------|----------------------------------------------|
| Function  | Indicates for which check these settings are |
| Datatypes | String                                       |
| Example   | "StaticDir.dir.hasFilesRecursive"            |
| Note      | Should not be changed                        |
<br>

-------------------------------------------------------------------------------

| Parameter | description                               |
|-----------|-------------------------------------------|
| Function  | Description of the check                  |
| Datatypes | String                                    |
| Example   | "Determine the number of files recursive" |
<br>

-------------------------------------------------------------------------------

| Parameter | lowLimit                                                   |
|-----------|------------------------------------------------------------|
| Function  | minimum number the specified check (e.g.: number of files) |
| Datatypes | int, float, long                                           |
| Example   | 1                                                          |
<br>

-------------------------------------------------------------------------------

| Parameter | highLimit                                                  |
|-----------|------------------------------------------------------------|
| Function  | maximum number the specified check (e.g.: number of files) |
| Datatypes | int, float, long                                           |
| Example   | 5                                                          |
<br>

-------------------------------------------------------------------------------

| Parameter | referenceValues                                                                          |
|-----------|------------------------------------------------------------------------------------------|
| Function  | Custom values used in the check (use depends on the check)<br>(e.g.: allowed extensions) |
| Datatypes | dyn_mixed                                                                                |
| Example   | ["pnl", "xml", ""]                                                                       |
<br>

-------------------------------------------------------------------------------

| Parameter | scorePoints                                   |
|-----------|-----------------------------------------------|
| Function  | Indicates how many points the check is worth. |
| Datatypes | int                                           |
| Example   | 1                                             |
<br>

-------------------------------------------------------------------------------

| Parameter | enabled                                   |
|-----------|-------------------------------------------|
| Function  | Indicates if the check is being performed |
| Datatypes | boolean                                   |
| Example   | true                                      |
<br>

-------------------------------------------------------------------------------
