<br/>
<div class='titlediv'>
  <h1 class='newstitle'><TMPL_UNLESS name>Nueva</TMPL_UNLESS> Autochanger </h1>
</div>
<div class='bodydiv'>
   Tiene que crear una Ubicaci�n, la cual deber� tener el mismo nombre.<br/><br/>

   <form name='form1' action="?" method='get'>
    <table>
     <tr><td>Nombre :</td>     
         <td>
          <select name='ach' class='formulaire' id='ach'>
<TMPL_LOOP devices><option value='<TMPL_VAR name>'><TMPL_VAR name></option></TMPL_LOOP>
          </select>
         </td>
     </tr>
     <tr><td>Pre-comando :</td> 
         <td> <input class="formulaire" type='text' id='precmd' value='sudo'
           title='can be "sudo" or "ssh storage\@storagehost"...' name='precmd'>
         </td>
     </tr>
     <tr><td>Comando mtx :</td> 
         <td> <input class="formulaire" type='text' name='mtxcmd' size='32'
               value='/usr/sbin/mtx' id='mtxcmd'>
         </td>
     </tr>
     <tr><td>Device :</td> 
         <td> <input class="formulaire" type='text' name='device' 
               value='/dev/changer' id='device'>
         </td>
     </tr>
    <tr><td><b>Drives</b></td><td/></tr>
    <TMPL_LOOP devices>
    <tr>
     <td><input class='formulaire' type='checkbox' id='drive_<TMPL_VAR name>'
                name='drives' value='<TMPL_VAR name>'><TMPL_VAR name>
     </td>
     <td>index <input type='text' title='drive index' 
                class='formulaire'
		id='index_<TMPL_VAR name>' value='' 
                name='index_<TMPL_VAR name>' size='3'>
     </td>
    </tr>
    </TMPL_LOOP>
    </table>
    <button type="submit" class="bp" name='action' value='ach_add'> <img src='/bweb/save.png' alt=''>Save</button>
   </form>
</div>

<script type="text/javascript" language="JavaScript">
  <TMPL_IF name>
  for (var i=0; i < document.form1.ach.length; ++i) {
     if (document.form1.ach[i].value == '<TMPL_VAR name>') {
        document.form1.ach[i].selected = true;
     }
  }
  </TMPL_IF>
  <TMPL_IF mtxcmd>
     document.getElementById('mtxcmd').value='<TMPL_VAR mtxcmd>';
  </TMPL_IF>
  <TMPL_IF precmd>
     document.getElementById('precmd').value='<TMPL_VAR precmd>';
  </TMPL_IF>
  <TMPL_IF device>
     document.getElementById('device').value='<TMPL_VAR device>';
  </TMPL_IF>
  <TMPL_IF drives>
   <TMPL_LOOP drives>
     document.getElementById('drive_<TMPL_VAR name>').checked=true;
     document.getElementById('index_<TMPL_VAR name>').value=<TMPL_VAR index>;
   </TMPL_LOOP>
  </TMPL_IF>
</script>
