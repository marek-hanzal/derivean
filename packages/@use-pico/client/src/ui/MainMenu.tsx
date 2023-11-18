import {type FC} from "react";
import {Menu}    from "./Menu";

export namespace MainMenu {
    export interface Props extends Menu.Props {
    }

    export type PropsEx = Omit<Props, "items">;
}

export const MainMenu: FC<MainMenu.Props> = props => {
    return <Menu
        {...props}
    />;
};
