import {PublicLayout as CoolPublicLayout} from "@use-pico2/ui-extra";
import {type FC}                          from "react";

export namespace PublicLayout {
    export type Props = CoolPublicLayout.Props;
}

export const PublicLayout: FC<PublicLayout.Props> = props => {
    return <CoolPublicLayout
        {...props}
    />;
};
