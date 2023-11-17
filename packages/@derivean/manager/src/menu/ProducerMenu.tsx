import {
    PipelineIcon,
    ProducerIcon
}                from "@derivean/ui";
import {Menu}    from "@use-pico/client";
import {t}       from "@use-pico/translator";
import {type FC} from "react";

export namespace ProducerMenu {
    export interface Props extends Menu.PropsEx {
        producerId: string;
    }
}

export const ProducerMenu: FC<ProducerMenu.Props> = (
    {
        producerId,
        ...props
    }
) => {
    const query = {
        id: producerId,
    };

    return <Menu
        items={[
            {
                type:  "link",
                href:  "/manager/producer/[id]",
                label: t()`Producer detail (label)`,
                icon:  <ProducerIcon/>,
                query,
            },
            {
                type:  "link",
                href:  "/manager/producer/[id]/pipeline",
                label: t()`Producer pipeline`,
                icon:  <PipelineIcon/>,
                query,
            },
        ]}
        {...props}
    />;
};
