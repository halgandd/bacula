
<div class='titlediv'>
  <h1 class='newstitle'> __Group:__ <TMPL_VAR client_group></h1>
</div>
<div class='bodydiv'>
   <form action="?" method='get'>
    <input type='hidden' name='client_group' value=<TMPL_VAR client_group>>
    <table>
     <tr><td>__Group:__</td>
         <td> 
          <input class="formulaire" type='text' value=<TMPL_VAR client_group> size='15' name='newgroup'> 
         </td>
     </tr>
     <tr><td>__Clients:__</td> 
         <td>
            <select name='client' size='15' class='formulaire' multiple>
            <TMPL_LOOP db_clients>
            <option id='client_<TMPL_VAR clientname>'><TMPL_VAR clientname></option>
            </TMPL_LOOP>
            </select>
         </td>
     </tr>
    </table>
    <button type="submit" class="bp" name='action' value='groups_save'>
     <img src='/bweb/save.png' alt=''>__Save__</button>
   </form>
</div>

<script type="text/javascript" language="JavaScript">
  <TMPL_LOOP client_group_member>
     document.getElementById('client_<TMPL_VAR name>').selected = true;
  </TMPL_LOOP>
</script>
