<br/>
<div class='titlediv'>
  <h1 class='newstitle'> Configuration </h1>
</div>
<div class='bodydiv'>
   <table>
    <tr>  <td><b>Connexion SQL</b></td>  <td/></tr>
    <tr><td>DBI :</td>      <td> <TMPL_VAR dbi>      </td></tr>
    <tr><td>user :</td>     <td> <TMPL_VAR user>     </td></tr>
    <tr><td>password :</td> <td> <TMPL_VAR password> </td></tr>
    <tr>  <td><b>Options G�n�rales</b></td>  <td/></tr>
    <tr><td>email_media :</td> <td> <TMPL_VAR email_media> </td></tr>
    <tr>  <td><b>Configuration Bweb</b></td>  <td/></tr>
    <tr><td title="/chemin/vers/votre/template_dir">template_dir :</td> <td> <TMPL_VAR template_dir> </td></tr>
    <tr><td title="/chemin/vers/une/font.ttf">graph_font :</td> <td> <TMPL_VAR graph_font> </td></tr>
    <tr><td title="/chemin/vers/bconsole -n -c /chemin/vers/bconsole.conf">bconsole :</td> <td> <TMPL_VAR bconsole> </td></tr>
    <tr><td>debug :</td> <td> <TMPL_VAR debug> </td></tr>
    <TMPL_IF achs>
    <tr>  <td><b>Robotique (Autochanger)</b></td>  <td/></tr>
    <tr>
     <td>
     <form action='?' method='GET'>
     <table border='0'>
    <TMPL_LOOP achs>
      <tr> 
       <td>
<label>
 <input type='radio' name='ach' value='<TMPL_VAR name>'><TMPL_VAR name>
</label>
       </td>
      </tr>
    </TMPL_LOOP>
    </table>
   <td>
  
   <input type="image" name="action" value="ach_edit" title="modifier" src='/bweb/edit.png'> 
   <input type="image" name='action' value='ach_del' title='supprimer' src='/bweb/remove.png'>
   <input type='image' name='action' value='ach_view' title='voir' src='/bweb/zoom.png'>
    </form>
    </td>
   </tr>
   </TMPL_IF achs>
   <tr>
   <td><hr></td><td></td>
   </tr>
  </table>

  <form action='?' method='GET'>
   <label>
   <input name='action' value='edit_conf' type="image" title='Modifier' src='/bweb/edit.png'> Modifier
   </label>
   <label>
   <input name='action' value='ach_add' type="image" title='Ajouter une robotique' src='/bweb/add.png'> Ajouter une robotique
   </label>
  </form>

  <TMPL_IF error>
  info :  <TMPL_VAR error> </br>
  </TMPL_IF>
</div>
