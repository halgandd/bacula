<br/>
<div class='titlediv'>
 <h1 class='newstitle'>Update medium <TMPL_VAR volumename></h1>
</div>
<div class='bodydiv'>
  <form name='form1' action="?" method='GET'>
   <table>
    <tr><td>Volume Name:</td>
        <td><input type='text' name='media' class='formulaire' value='<TMPL_VAR volumename>' title='Change this to update an other volume'>
        </td>
    </tr>
    <tr><td>Pool:</td>
        <td><select name='pool' class='formulaire'>
<TMPL_LOOP db_pools>
             <option value='<TMPL_VAR name>'><TMPL_VAR name></option>
</TMPL_LOOP>
           </select>
        </td>
    </tr>
    <tr><td>Estado:</td>
        <td><select name='volstatus' class='formulaire'>
           <option value='Append'>Append</option>
           <option value='Archive'>Archive</option>
           <option value='Disabled'>Disabled</option>
           <option value='Cleaning'>Cleaning</option>
           <option value='Error'>Error</option>
	   <option value='Full'>Full</option>
           <option value='Read-Only'>Read-Only</option>
           <option value='Used'>Used</option>
	   <option value='Recycle'>Recycle</option>
	   <option value='Purged'>Purged</option>
           </select>
        </td>
    </tr>

    <tr><td>Slot:</td>
        <td> 
          <input class='formulaire' type='text' 
                 name='slot' value='<TMPL_VAR slot>'>
        </td>
    </tr>

    <tr><td>Cargado:</td>
        <td> 
          <input class='formulaire' type='checkbox' 
               name='inchanger' <TMPL_IF inchanger>checked</TMPL_IF>>
        </td>
    </tr>

    <tr><td>Activado :</td>
        <td> <select name='enabled' class='formulaire'>
           <option value='yes'>yes</option>
           <option value='no'>no</option>
           <option value='archived'>archived</option>
           </select>
        </td>
    </tr>

    <tr><td> Ubicaci�n : </td>
        <td><select name='location' class='formulaire'>
        <option value=''></option>
  <TMPL_LOOP db_locations>
      <option value='<TMPL_VAR location>'><TMPL_VAR location></option>
  </TMPL_LOOP>
    </select>
        </td>
    </tr>
    <tr><td> Per�odo de Retenci�n: </td>
        <td>
          <input class='formulaire' type='text' title='ex: 3 days, 1 month'
               name='volretention' value='<TMPL_VAR volretention>'>
        </td>
    </tr>
    <tr><td> Use duration: </td>
        <td>
          <input class='formulaire' type='text' title='ex: 3 days, 1 month'
               name='voluseduration' value='<TMPL_VAR voluseduration>'>
        </td>
    </tr>
    <tr><td> Jobs M�ximos: </td>
        <td>
          <input class='formulaire' type='text' title='ex: 10'
               name='maxvoljobs' value='<TMPL_VAR maxvoljobs>'>
        </td>
    </tr>
    <tr><td> Archivos M�ximos: </td>
        <td>
          <input class='formulaire' type='text' title='ex: 10000'
               name='maxvolfiles' value='<TMPL_VAR maxvolfiles>'>
        </td>
    </tr>
    <tr><td> Bytes M�ximos: </td>
        <td>
          <input class='formulaire' type='text' title='ex: 10M, 11G'
               name='maxvolbytes' value='<TMPL_VAR maxvolbytes>'>
        </td>
    </tr>
    <tr><td>Recycle Pool:</td>
        <td><select name='poolrecycle' class='formulaire'>
<TMPL_LOOP db_pools>
             <option value='<TMPL_VAR name>'><TMPL_VAR name></option>
</TMPL_LOOP>
           </select>
        </td>
    </tr>
    <tr><td> Comentario: </td>
        <td>
          <input class='formulaire' type='text' title='a comment'
               name='comment' value='<TMPL_VAR comment>'>
        </td>
    </tr>

    </table>
<table>
 <td>
  <button type="submit" class="bp" name='action' value='do_update_media'> <img src='/bweb/apply.png' alt=''> Apply </button>
  <button type="submit" class="bp" name='action' title='Actualizar del Pool'
    value='update_from_pool'> <img src='/bweb/update.png' alt=''> Actualizar del Pool </button>
 </form>
 </td>
 <td>
  <form action='?' method='GET'>
   <input type='hidden' name='pool' value='<TMPL_VAR poolname>'>
    <button type="submit" class="bp" name='action' value='media'>
     <img src='/bweb/zoom.png' alt=''>Ver Pool </button>
  </form>
 </td>
</table>
</div>

<script type="text/javascript" language='JavaScript'>
var ok=1;
for (var i=0; ok && i < document.form1.pool.length; ++i) {
   if (document.form1.pool[i].value == '<TMPL_VAR poolname>') {
      document.form1.pool[i].selected = true;
      ok=0;
   }
}

ok=1;
for (var i=0; ok && i < document.form1.pool.length; ++i) {
   if (document.form1.poolrecycle[i].value == '<TMPL_VAR poolrecycle>') {
      document.form1.poolrecycle[i].selected = true;
      ok=0;
   }
}

ok=1;
for (var i=0; ok && i < document.form1.location.length; ++i) {
   if (document.form1.location[i].value == '<TMPL_VAR location>') {
      document.form1.location[i].selected = true;
      ok=0;
   }
}

ok=1;
for (var i=0; ok && i < document.form1.volstatus.length; ++i) {
   if (document.form1.volstatus[i].value == '<TMPL_VAR volstatus>') {
      document.form1.volstatus[i].selected = true;
      ok=0;
   }
}
ok=1;
for (var i=0; ok && i < document.form1.enabled.length; ++i) {
   if (document.form1.enabled[i].value == '<TMPL_VAR enabled>') {
      document.form1.enabled[i].selected = true;
      ok=0;
   }
}

</script>
