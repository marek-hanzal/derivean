import {
    type FC,
    type ReactNode
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {type IHrefProps}  from "../api/IHrefProps";

export namespace ButtonLink {
    export interface Props extends CommonProps {
        icon?: ReactNode;
        href: IHrefProps | string;
        label?: ReactNode;
    }
}

export const ButtonLink: FC<ButtonLink.Props> = () => {
    return "ButtonLink";
};
