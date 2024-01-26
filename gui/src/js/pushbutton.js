function Button(unique,title,color=null,bg_color=null){
    var div_id = "box_"+String(unique);
    var label = "On";
    var color = color;
    var title = String(title);
    var bg_color = bg_color;
    var value; //holds toggle value right now
    var unique = String(unique); //unique identifying number
    var overall_div = document.getElementById(div_id);
    var holder;
    var button_element;
    var setup = function(){
        holder = document.createElement("div");
        holder.setAttribute("id", div_id+unique+"_holder");
        holder.setAttribute("class", "button_holder");

        // fix for when we want to allow dragging
        // but also button clicking
        // without collisions
        var topHandle = document.createElement("div");
        topHandle.setAttribute("class", "handle top_handle");
        holder.appendChild(topHandle)
        var leftHandle = document.createElement("div");
        leftHandle.setAttribute("class", "handle left_handle");
        holder.appendChild(leftHandle)

        button_element = document.createElement("button");
        button_element.setAttribute("class","gui_button");
        button_element.setAttribute("id",div_id+unique+"button");
        button_element.innerHTML = title;
        holder.appendChild(button_element);

        var rightHandle = document.createElement("div");
        rightHandle.setAttribute("class", "handle right_handle");
        holder.appendChild(rightHandle)
        var bottomHandle = document.createElement("div");
        bottomHandle.setAttribute("class", "handle bottom_handle");
        holder.appendChild(bottomHandle)

        overall_div.appendChild(holder);
           
        if (bg_color===null || color===null){
            console.log("no color");
        }else{
            button_element.setAttribute("style","background-color:"+bg_color+";color: "+color);
        }
        //$("#"+div_id+unique+"_holder").trigger("create");
    }
    setup();

    this.update = function(f){
        if (typeof f == "boolean") f = (f=="true");
        if(f) button_element = true;
        else button_element = false;
    }

    button_element.addEventListener("mousedown",function(){
        var local_change = new CustomEvent('ui_change',{detail:{"message":String(unique)+":"+String(true)}});
        document.dispatchEvent(local_change);
        //var local_change = new CustomEvent('ui_change',{unique:"True"});
        //document.dispatchEvent(local_change);        
    });
    button_element.addEventListener('mouseup',function(){
        var local_change = new CustomEvent('ui_change',{detail:{"message":String(unique)+":"+String(false)}});
        document.dispatchEvent(local_change);
    });

};
