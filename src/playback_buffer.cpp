#include "playback_buffer.h"

playback_buffer::playback_buffer()
{
    //ctor
    length = 0; //current length of playback buffer
	max_length = MAX_PBBFR_SIZE; //maximum length of playback buffer
	last_packet = first_packet = NULL;
}

playback_buffer::~playback_buffer()
{
    //dtor
    clear_pbb();
}

int playback_buffer::copy_packet (pbb_packet *from, pbb_packet *to)
{
    //copy all fields from one pbb_packet to another
	to->next = from->next;
	int i;
	for (i = 0; i < from->pl_size; i++)
		to->data[i] = from->data[i];
	to->nr = from->nr;
	to->nt = from->nt;
	to->p_t.frame = from->p_t.frame;
	to->p_t.pred_num = from->p_t.pred_num;
	to->pl_size = from->pl_size;
	to->gop_num = from->gop_num;
	to->p2p_pn = from->p2p_pn;
	to->pn = from->pn;
	return EXIT_SUCCESS;
}

bool playback_buffer::shift_buffer()
{
    //shift a buffer if it's time
    if (length >= max_length)
	{
        add_packet(&new_packet);
        delete_first_packet();
        return true;
    }
	else
	{
    	add_packet(&new_packet);
    	return false;
	}
}

bool playback_buffer::shift_buffer (int s, struct sockaddr_in addr, unsigned long &pn)
{
    //shift a buffer and send first packet if it's time
	if (length >= max_length)
	{
	    pn = first_packet->pn; //first packet number for statistics
		send_first_packet (s, addr);
        add_packet(&new_packet);
        return true;
    }
	else
	{
    	add_packet(&new_packet);
    	return false;
	}
}

int playback_buffer::delete_first_packet()
{
	pbb_packet *temp;
	if (first_packet == NULL)
	{
		length = 0;
		return EXIT_SUCCESS;
	}
	else if (first_packet->next == NULL)
	{
		delete (first_packet);
		length = 0;
		return EXIT_SUCCESS;
	}
	else
	{
	    temp = first_packet->next;
	    delete (first_packet);
	    first_packet = NULL;
	    first_packet = temp;
		length--;
		if (first_packet == NULL)
			length = 0;
		return EXIT_SUCCESS;
	}
}

int playback_buffer::send_first_packet (int s, struct sockaddr_in addr)
{
	sendto(s, (char *) first_packet->data, first_packet->pl_size, 0, (struct sockaddr *) &addr, sizeof (struct sockaddr));
    delete_first_packet();
    return EXIT_SUCCESS;
}

int playback_buffer::add_packet (pbb_packet *packet)
{
	pbb_packet 	*temp = new pbb_packet,
				*cur,
		   		*prev;
	if (length == 0)
	{
		copy_packet (&new_packet, temp);
		first_packet = temp;
		last_packet = temp;
	}
	else if (first_packet->pn > packet->pn)
	{
		copy_packet (first_packet, temp);
		copy_packet (&new_packet, first_packet);
		first_packet->next = temp;
	}
	else if (last_packet->pn < packet->pn)
	{
		copy_packet (&new_packet, temp);
		last_packet->next = temp;
		last_packet = temp;
	}
	else
	{
		copy_packet (&new_packet, temp);
		prev = first_packet;
		cur = first_packet->next;
		while ((cur != NULL) && (cur->pn < packet->pn))
		{
			prev = cur;
			cur = prev->next;
		}
		if (cur->pn == packet->pn)
			return EXIT_FAILURE;
		prev->next = temp;
		temp->next = cur;
	}
	length++;
	return EXIT_SUCCESS;
}

int playback_buffer::get_packet_by_p2p_pn (unsigned long p2p_pn, unsigned char nt)
{
	pbb_packet	*cur,
		   		*prev;
	if (length == 0 || first_packet == NULL)
	{
		return EXIT_FAILURE;
	}
	else if ((first_packet->nt == nt) && (first_packet->p2p_pn == p2p_pn))
    {
        copy_packet (first_packet, &new_packet);
        return EXIT_SUCCESS;
    }
	else
	{
		prev = first_packet;
		cur = first_packet->next;
        while (cur != NULL)
		{
		    if ((cur->nt == nt) && (cur->p2p_pn == p2p_pn))
                break;
            prev = cur;
            cur = prev->next;
		}
		if (cur != NULL)
		{
			copy_packet (cur, &new_packet);
			return EXIT_SUCCESS;
		}
		else
		{
			return EXIT_FAILURE;
		}
	}
}

int playback_buffer::get_pn_by_p2p_pn (unsigned long p2p_pn, unsigned char nt, unsigned long &pn)
{
    pbb_packet	*cur,
		   		*prev;
	if (length == 0 || first_packet == NULL)
	{
		return EXIT_FAILURE;
	}
	else if ((first_packet->nt == nt) && (first_packet->p2p_pn == p2p_pn))
    {
        pn = first_packet->pn;
        return EXIT_SUCCESS;
    }
	else
	{
		prev = first_packet;
		cur = first_packet->next;
        while (cur != NULL)
		{
		    if ((cur->nt == nt) && (cur->p2p_pn == p2p_pn))
                break;
            prev = cur;
            cur = prev->next;
		}
		if (cur != NULL)
		{
		    pn = cur->pn;
			return EXIT_SUCCESS;
		}
		else
		{
			return EXIT_FAILURE;
		}
	}
}

int playback_buffer::get_packet_by_pn (unsigned long pn)
{
	pbb_packet	*cur,
		   		*prev;
	if (length == 0 || first_packet == NULL)
	{
		return EXIT_FAILURE;
	}
	else if (first_packet->pn > pn)
	{
		return EXIT_FAILURE;
	}
	else if (first_packet->pn == pn)
	{
		copy_packet (first_packet, &new_packet);
		return EXIT_SUCCESS;
	}
	else if ((new_packet.next != NULL) && (new_packet.next->pn == pn))
	{
		copy_packet (new_packet.next, &new_packet);
		return EXIT_SUCCESS;
	}
	else if (last_packet->pn < pn)
	{
		return EXIT_FAILURE;
	}
	else if ((new_packet.next != NULL) && (new_packet.next->pn < pn))
	{
		prev = new_packet.next;
		cur = new_packet.next->next;
		while ((cur != NULL) && (cur->pn < pn))
		{
			prev = cur;
			cur = prev->next;
		}
		if ((cur != NULL) && (cur->pn == pn))
		{
			copy_packet (cur, &new_packet);
		}
		else
		{
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}
	else
	{
		prev = first_packet;
		cur = first_packet->next;
		while ((cur != NULL) && (cur->pn < pn))
		{
			prev = cur;
			cur = prev->next;
		}
		if ((cur != NULL) && (cur->pn == pn))
		{
			copy_packet (cur, &new_packet);
		}
		else
		{
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}
}

int playback_buffer::clear_pbb ()
{
	while (length != 0)
		delete_first_packet();
	length = 0;
	return EXIT_SUCCESS;
}

int playback_buffer::show_pn ()
{
	pbb_packet *cur = first_packet;
	int i;
	printf (" PBB%d: ", length);
	for (i = 0; i < length; i++)
	{
		if (cur != NULL)
		{
			printf ("%lu;", cur->pn);
			cur = cur->next;
		}
	}
	printf ("\n");
	return EXIT_SUCCESS;
}
