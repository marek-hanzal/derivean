"use client";

import {KingdomIcon}       from "@derivean/ui";
import {t}                 from "@use-pico/i18n";
import {ButtonLink}        from "@use-pico/ui";
import {
    type ComponentProps,
    type FC
}                          from "react";
import {KingdomUpsertForm} from "../form/KingdomUpsertForm";
import {KingdomInline}     from "../inline/KingdomInline";
import {KingdomRpc}        from "../rpc/KingdomRpc";
import {KingdomUI}         from "../ui/KingdomUI";

export namespace KingdomTable {
    export type Columns =
        | "name";

    export type Props = Omit<
        ComponentProps<typeof KingdomUI.Table<Columns>>,
        "columns" | "name" | "icon" | "text"
    >
}

export const KingdomTable: FC<KingdomTable.Props> = props => {
    return <KingdomUI.Table
        text={{
            total: t()`Total count of kingdoms`,
            count: {
                empty: {
                    title:   t()`No kingdoms found`,
                    message: t()`There are not kingdoms yet`,
                },
            }
        }}
        name={"kingdom"}
        icon={<KingdomIcon/>}
        tableActionProps={{
            text:       {
                create: {
                    title: t()`Create kingdom (modal)`,
                    label: t()`Create kingdom`,
                },
            },
            upsertForm: ({modalId}) => <KingdomUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            text:         {
                update: {
                    title: t()`Update kingdom`,
                    label: t()`Update kingdom`,
                },
                delete: {
                    label: t()`Delete kingdom`,
                    modal: {
                        title:   t()`Delete kingdom (modal)`,
                        content: t()`Do you really want to delete selected kingdom?`,
                        success: {
                            title:   t()`Success`,
                            message: t()`Kingdom has been successfully removed`,
                        },
                    },
                },
            },
            withMutation: KingdomRpc.mutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <KingdomUpsertForm
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            name: {
                title:  t()`Kingdom name`,
                render: ({item}) => <ButtonLink
                    icon={<KingdomIcon/>}
                    href={{
                        href:  "/game/kingdom/[id]",
                        query: {
                            id: item.id,
                        },
                    }}
                    label={<KingdomInline entity={item}/>}
                />,
            },
        }}
        {...props}
    />;
};
