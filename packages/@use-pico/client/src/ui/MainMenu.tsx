import {type FC} from "react";
import {Menu}    from "./Menu";

export namespace MainMenu {
    export interface Props extends Menu.Props {
    }
}

export const MainMenu: FC<MainMenu.Props> = props => {
    return <Menu
        {...props}
    />;
};
