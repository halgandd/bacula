<br/>
<div class='titlediv'>
  <h1 class='newstitle'> Localisation : <TMPL_VAR Location></h1>
</div>
<div class='bodydiv'>
   <form action="?" method='get'>
    <input type='hidden' name='location' value='<TMPL_VAR location>'>
    <table>
     <tr><td>Localisation :</td>     
         <td> 
          <input class="formulaire" type='text' value='<TMPL_VAR location>' size='32' name='newlocation'> 
         </td>
     </tr>
     <tr><td>Co�t :</td> 
         <td> <input class="formulaire" type='text' value='<TMPL_VAR cost>' name='cost' size='3'>
         </td>
     </tr>
     <tr><td>Enabled :</td> 
         <td> <input class="formulaire" type='checkbox' name='enabled' <TMPL_IF enabled> checked </TMPL_IF> >
         </td>
     </tr>
    </table>
    <input type="image" name='action' value='location_save'
     src='/bweb/save.png'>
   </form>
</div>
