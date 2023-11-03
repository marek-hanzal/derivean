import {TableActionMenu} from "@use-pico/table";
import {
    CreateIcon,
    Modal,
    ModalMenuItem
}                        from "@use-pico/ui";
import {
    type ReactNode,
    useMemo
}                        from "react";

export namespace TableAction {
    export interface Props {
        name: string;
        icon: ReactNode;
        upsertForm: UpsertFormFactory;
    }

    export type UpsertFormFactory = (props: UpsertFormFactory.Props) => ReactNode;
    export namespace UpsertFormFactory {
        export interface Props {
            modalId: string;
        }
    }
}

export const TableAction = (
    {
        name,
        icon,
        upsertForm,
    }: TableAction.Props
) => {
    const createId = `${name}.create`;
    return <>
        <Modal
            icon={<CreateIcon/>}
            modalId={createId}
            title={"create.title"}
            modalProps={{
                closeOnClickOutside: false,
            }}
        >
            {useMemo(() => upsertForm({
                modalId: createId,
            }), [name])}
        </Modal>

        <TableActionMenu>
            <ModalMenuItem
                modalId={createId}
                withLabel={"create.label"}
                leftSection={icon}
            />
        </TableActionMenu>
    </>;
};
