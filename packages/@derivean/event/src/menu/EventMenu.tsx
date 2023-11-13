import {Menu}    from "@use-pico/ui";
import {type FC} from "react";

export namespace EventMenu {
    export interface Props extends Menu.PropsEx {
    }
}

export const EventMenu: FC<EventMenu.Props> = props => {
    return <Menu
        items={[]}
        {...props}
    />;
};
