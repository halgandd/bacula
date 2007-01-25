<br/>
 <div class='titlediv'>
  <h1 class='newstitle'>Move media</h1>
 </div>
 <div class="bodydiv">
   <form action='?' method='get'>
    <table id='id<TMPL_VAR NAME=ID>'></table>
    <table border='0'>
    <tr><td> New location: </td><td>
<select name='newlocation' class='formulaire'>
    <TMPL_LOOP NAME=db_locations>
    <option value='<TMPL_VAR NAME=location>'><TMPL_VAR NAME=location></option>
    </TMPL_LOOP>
</select>
    </td></tr><tr><td> Status: </td><td>
<select name='volstatus' class='formulaire'>
    <option value=''>Don't update</option>
    <option value='Append'>Append</option>
    <option value='Archive'>Archive</option>
    <option value='Disabled'>Disabled</option>
    <option value='Cleaning'>Cleaning</option>
    <option value='Error'>Error</option>
    <option value='Full'>Full</option>
    <option value='Purged'>Purged</option>
    <option value='Read-Only'>Read-Only</option>
    <option value='Recycle'>Recycle</option>
    <option value='Used'>Used</option>
</select>
    </td><tr><td> User: </td><td>
<input type='text' name='user' value='<TMPL_VAR loginname>' class='formulaire'>
    </td></tr>
    </td></tr><tr><td> Comment: </td><td>
<textarea name="comment" class='formulaire'></textarea>
    </td></tr>
    </table>
    <label>
    <input type="image" type='submit' name='action' value='change_location' src='/bweb/apply.png'> Move
    </label>
   </form>
 </div>

<script type="text/javascript" language="JavaScript">

var header = new Array("Volume Name", "Location", "Select");

var data = new Array();
var chkbox;

<TMPL_LOOP NAME=medias>
chkbox = document.createElement('INPUT');
chkbox.type  = 'checkbox';
chkbox.value = '<TMPL_VAR name=volumename>';
chkbox.name  = 'media';
chkbox.checked = 1;

data.push( new Array(
"<TMPL_VAR NAME=volumename>",
"<TMPL_VAR NAME=location>",
chkbox
 )
);
</TMPL_LOOP>

nrsTable.setup(
{
 table_name:     "id<TMPL_VAR NAME=ID>",
 table_header: header,
 table_data: data,
 up_icon: up_icon,
 down_icon: down_icon,
 prev_icon: prev_icon,
 next_icon: next_icon,
 rew_icon:  rew_icon,
 fwd_icon:  fwd_icon,
// natural_compare: false,
 even_cell_color: even_cell_color,
 odd_cell_color: odd_cell_color, 
 header_color: header_color,
 page_nav: true,
 padding: 3,
// disable_sorting: new Array(5,6),
 rows_per_page: rows_per_page
}
);
</script>
