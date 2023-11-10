import {IconSubtask} from "@tabler/icons-react";
import {
    t,
    tx
}                    from "@use-pico/i18n";
import {Page}        from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<IconSubtask/>}
        text={{
            title:  tx()`Game management`,
            header: t()`Game management`,
        }}
    >
    </Page>;
}
