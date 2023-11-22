import {type FC} from "react";
import {Icon}    from "../ui/Icon";

export namespace withIcon {
    export interface Props {
        icon: string;
    }
}

export const withIcon = (
    {
        icon,
    }: withIcon.Props
): FC<Omit<Icon.Props, "icon">> => {
    return props => <Icon icon={icon} {...props}/>;
};
