<br/>
<div class='titlediv'>
  <h1 class='newstitle'> __Location:__ <TMPL_VAR Location></h1>
</div>
<div class='bodydiv'>
   <form name='form1' action="?" method='get'>
    <input type='hidden' name='location' value='<TMPL_VAR location>'>
    <table>
     <tr><td>__Location:__</td>     
         <td> 
          <input class="formulaire" type='text' value='<TMPL_VAR location>' size='32' name='newlocation'> 
         </td>
     </tr>
     <tr><td>__Cost:__</td> 
         <td> <input class="formulaire" type='text' value='<TMPL_VAR cost>' name='cost' size='3'>
         </td>
     </tr>
    <tr><td>__Enabled:__</td>
        <td> <select name='enabled' class='formulaire'>
           <option value='yes'>__yes__</option>
           <option value='no'>__no__</option>
           <option value='archived'>__archived__</option>
           </select>
        </td>
    </tr>
    </table>
    <button type="submit" class="bp" name='action' value='location_save'>
      <img src='/bweb/save.png' alt=''>__Save__</button>
   </form>
</div>

<script type="text/javascript" language='JavaScript'>
ok=1;
for (var i=0; ok && i < document.form1.enabled.length; ++i) {
   if (document.form1.enabled[i].value == '<TMPL_VAR enabled>') {
      document.form1.enabled[i].selected = true;
      ok=0;
   }
}
</script>
