import {PublicLayout as CoolPublicLayout} from "@use-pico/client";
import {type FC}                          from "react";

export namespace PublicLayout {
    export type Props = CoolPublicLayout.Props;
}

export const PublicLayout: FC<PublicLayout.Props> = props => {
    return <CoolPublicLayout
        {...props}
    />;
};
